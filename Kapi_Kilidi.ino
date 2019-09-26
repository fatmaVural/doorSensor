 
#include <EEPROM.h>     //Okuma ve yazma işini UID ile EEPROM a kaydedicez.
#include <SPI.h>        //RC522 modülü SPI protokolünü kullanır.
#include <MFRC522.h>    //Mifare RC522 kütüphanesi

#define COMMON_ANODE     //kullandığınız ledlere göre yanık veya sönük(H-L) kodlarını düzenleyiniz.
#ifdef COMMON_ANODE
#define LED_ON HIGH
#define LED_OFF LOW
#else
#define LED_ON LOW
#define LED_OFF HIGH
#endif

#define redLed 7   // Led pinlerini seçtik.
#define greenLed 6
#define yellowLed 5

#define role 4    //Röle pinini seçtik.
#define format 3    //Master Card'ı silmek için buton pinini seçtik.

boolean match=false;   
boolean programMode=false;  //Programlama modu başlatma.

int basariliOkuma;    //Başarılı bir şekilde sayı okuyabilmek için integer atıyoruz.

byte storedCard[4];   //Kart EEPROM tarafından okundu.
byte readCard[4];     //RFID modül ile ID tarandı.
byte masterCard[4];   //Master kart ID'si EEPROM'a aktarıldı.

/*
  MOSI: Pin 11 / ICSP-4
  MISO: Pin 12 / ICSP-1
  SCK : Pin 13 / ICSP-3
  SS(SDA) : Pin 10 (Configurable)
  RST : Pin 9 (Configurable)
*/

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN,RST_PIN);  //MFRC522 örneği oluşturduk.

///////////////////////YÜKLEME/////////////////////

void setup()
{
  //Arduino Pin Konfigrasyonu
  pinMode(redLed,OUTPUT);
  pinMode(greenLed,OUTPUT);
  pinMode(yellowLed,OUTPUT);
  pinMode(format,INPUT_PULLUP);
  pinMode(role,OUTPUT);
  
  digitalWrite(role,LOW);        //Kapının kilitlidir.
  digitalWrite(redLed,LED_OFF);    //Ledin sönük olduğundan emin olun.
  digitalWrite(greenLed,LED_OFF);  //Ledin sönük olduğundan emin olun.
  digitalWrite(yellowLed,LED_OFF);   //Ledin sönük olduğundan emin olun.
  //Protokol konfigrasyonu
  Serial.begin(9600);    //PC ile seri iletim başlat.
  SPI.begin();           //MFRC522 donanımı SPI protokolünü kullanır.
  mfrc522.PCD_Init();    //MFRC522 donanımını başlat.

  Serial.println(F("Kartla Arac Calistirma v1.0"));  //Hata ayıklama amacıyla
  OkuyucuDetayGoster();   //PCD ve MFRC522 kart okuyucu ayrıntılarını göster.


  //Silme kodu butona basıldığında EEPROM içindeki bilgileri siler.
  if(digitalRead(format)==LOW)
  {
    digitalWrite(redLed,LED_ON); //Kırmızı led sildiğimizi bilgilendirmek amacıyla yanık kalır.
    Serial.println(F("Silme butonuna basildi."));
    Serial.println(F("5 saniye icinde iptal edebilirsiniz.."));
    Serial.println(F("Tum kayitlar silinecek ve bu islem geri alinamayacak."));
    delay(5000);                       //Kullanıcıya iptal işlemi için yeterli zaman verin.
    if (digitalRead(format)==LOW)       //Düğmeye basılmaya devam ediliyorsa EEPROM u sil.
    {
      Serial.println(F("EEPROM silinmeye baslaniyor."));
      for (int x=0; x<EEPROM.length();x=x+1)  //EEPROM adresinin döngü sonu
      {
        if(EEPROM.read(x)==0)   //EEPROM adresi sıfır olursa
        { }
        else
        {
          EEPROM.write(x,0);
        }
       }
       Serial.println(F("EEPROM Basariyla Silindi.."));
       digitalWrite(redLed,LED_OFF);
       delay(200);
       digitalWrite(redLed,LED_ON);
       delay(200);
       digitalWrite(redLed,LED_OFF);
       delay(200);
       digitalWrite(redLed,LED_ON);
       delay(200);
       digitalWrite(redLed,LED_OFF);
       }
       else
       {
        Serial.println(F("Silme islemi iptal edildi."));
        digitalWrite(redLed,LED_OFF);
       }
      }

/*Master Kart tanımlı mı kontrol edilecek.Eğer kullanıcı Master Kart seçmediyse      
  yeni bir Master Kart EEPROM a kaydedilecek.EEPROM kayıtlarının haricinde 143 EEPROM adresi tutulabilir.
 */

 if (EEPROM.read(1) != 143)
 {
  Serial.println(F("Master Kart Secilmedi."));
  Serial.println(F("Master Karti secmek icin kartinizi okutunuz.."));
  do
  {
    basariliOkuma=getID();             //basariliOkuma 1 olduğu zaman okuyucu düzenlenir aksi halde 0 olucaktır.
    digitalWrite(yellowLed,LED_ON);    //Master Kartın kaydedilmesi için gösterildiğini ifade eder.
    delay(200);
    digitalWrite(yellowLed,LED_OFF);
    delay(200);
  }
  while(!basariliOkuma);              //Başarılı bir şekilde okuyamadıysa Başarılı okuma işlemini yapmicaktır.
  for (int j=0; j<4; j++)           //4 kez döngü
  {
    EEPROM.write(2+j,readCard[j]);    //UID EPPROM a yazıldı, 3. adres başla.
  }
  EEPROM.write(1,143);      //EEPROM a Master Kartı kaydettik.
  Serial.println(F("Master Kart kaydedildi.."));
  }

Serial.println(F("*****************"));
Serial.println(F("Master Kart UID:"));
for(int i=0; i<4; i++)                    //EEPROM da Master Kartın UID'si okundu.
{                                         //Master Kart yazıldı.
  masterCard[i]=EEPROM.read(2+i);
  Serial.print(masterCard[i],HEX);
}

Serial.println("");
Serial.println(F("***************"));
Serial.println(F("Hersey Hazir!!"));
Serial.println(F("Kart okutulmasi icin bekleniyor..."));
ledDongu();                                         //Herşeyin hazır olduğunu kullanıcıya haber vermek için geri bildirim.
}


void loop()
{


  do
  {
  
    basariliOkuma=getID();       //basariliOkuma 1 olursa kartı oku aksi halde 0
    if(programMode)
    {
      ledDongu();              //Program modu geri bildirimi yeni kart okutulmasını bekliyor.
    }
    else
    {
      normalModeOn();        //Normal mod, sarı led açık diğer tümü kapalı.
    }
  }
  while(!basariliOkuma);   
  if(programMode)
  {
    if(MasterDogrulama(readCard))    //Master Kart tekrar okutulursa programdan çıkar.
    {
      Serial.println(F("Master Kart Okundu.."));
      Serial.println(F("Programdan cikis yapiliyor.."));
      Serial.println(F("*********************"));
      programMode=false;
      return;
    }
    else
    {
      if(IDbul(readCard))   //Okunan kart silinmek isteniyorsa
      {
        Serial.println(F("Okunan kart siliniyor.."));
        IDsilme(readCard);
        Serial.println(F("****************"));
      }
      else                   //Okunan kart kaydedilmek isteniyorsa
      {
        Serial.println(F("Okunan kart hafizaya kaydediliyor.."));
        IDyazma(readCard);
        Serial.println(F("**************************"));
      }
    }
  }
  else
  {
    if(MasterDogrulama(readCard))   //Master Kart okunursa programa giriş yapılıyor.
    {
      programMode=true;
      Serial.println(F("Merhaba DUSCART UYESI, Programa giris yapiyorum."));
      int count=EEPROM.read(0);
      Serial.print(F("Sahip oldugum kullanici "));
      Serial.print(count);
      Serial.print(F("sayisi kadardir."));
      Serial.println("");
      Serial.println(F("Eklemek veya cikarmak istediginiz karti okutunuz."));
      Serial.println(F("*****************************"));
    }
    else
    {
     
      if(IDbul(readCard))
      {
        while(true)
        {
          Serial.println(F("Hosgeldin, gecis izni verildi."));
        
          digitalWrite(yellowLed,LED_OFF);  //sarı led sönük
  digitalWrite(redLed,LED_OFF);   //kırmızı led sönük
  digitalWrite(greenLed,LED_ON);  //yeşil led yanık
  digitalWrite(role,HIGH);        //kapı kilidi açıldı
 
  
   if(getID())
   break;
 
      }
         
      }
      else
      {
        Serial.println(F("Gecis izni verilmedi."));
        reddet();
      }
      
    }
  }
}

////////////////ERİŞİM İZNİ VERİLMEDİ/////////////////////

void reddet()
{
  digitalWrite(greenLed,LED_OFF);   //yeşil ledin sönük olduğundan emin olun
  digitalWrite(yellowLed,LED_OFF);    //sarı ledin sönük olduğuna emin olun
  digitalWrite(redLed,LED_ON);      //kırmızı led yanık
  delay(1000);
}

/////////////////////KART OKUYUCU UID AYARLAMA////////////////////////////

int getID()
{
  //Kart okuyucuyu hazır ediyoruz
  if(!mfrc522.PICC_IsNewCardPresent())   //yeni bir kart okutun vve devam edin.
  {
    return 0;
  }
  if(!mfrc522.PICC_ReadCardSerial())     //kartın serial numarasını alın ve devam edin.
  {
    return 0;
  }
  //4 ve 7 byte UID'ler mevcut biz 4 byte olanı kullanıcaz
  Serial.println(F("Kartin UID'sini taratin..."));
  for (int i=0; i<4; i++)
  {
    readCard[i]=mfrc522.uid.uidByte[i];
    Serial.print(readCard[i],HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); //Okuma durduruluyor.
  return 1;
}

void OkuyucuDetayGoster() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (bilinmeyen)"));
  Serial.println("");
  
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("DİKKAT! Haberlesme yetmezligi, MFRC522'yi dogru bagladiginizdan emin olun."));
    while(true);  
  }
}


//////////////////Cycle Leds(Program Modu)///////////////////////

void ledDongu() {
  digitalWrite(redLed, LED_OFF);   // kırmızı ledin sönük olduğundan emin olun
  digitalWrite(greenLed, LED_ON);   // yeşil ledin yanık olduğundan emin olun
  digitalWrite(yellowLed, LED_OFF);   // sarı ledin sönük olduğundan emin olun
  delay(200);
  digitalWrite(redLed, LED_OFF);  // kırmızı ledin sönük olduğundan emin olun
  digitalWrite(greenLed, LED_OFF);  // yeşil ledin sönük olduğundan emin olun
  digitalWrite(yellowLed, LED_ON);  // sarı ledin yanık olduğundan emin olun
  delay(200);
  digitalWrite(redLed, LED_ON);   // kırmızı ledin yanık olduğundan emin olun
  digitalWrite(greenLed, LED_OFF);  // yeşil ledin sönük olduğundan emin olun
  digitalWrite(yellowLed, LED_OFF);   // sarı ledin sönük olduğundan emin olun
  delay(200);
}


/////////////////Normal Led Modu//////////////////////////////

void normalModeOn () {
  digitalWrite(yellowLed, LED_ON);   // sarı led yanık ve program kart okumaya hazır
  digitalWrite(redLed, LED_OFF);  // kırmızı ledin sönük olduğundan emin olun
  digitalWrite(greenLed, LED_OFF);  // yeşil ledin sönük olduğundan emin olun
  digitalWrite(role,LOW );    //* kapının kilitli olduğundan emin olun
}

///////////////////////EEPROM için ID Okuma////////////////////////

void readID( int numara )
{
  int baslat=(numara*4)+2;    //başlama pozisyonu
  for (int i=0; i<4; i++)     //4 byte alamabilmek için 4 kez döngü kurucaz
  {
    storedCard[i]=EEPROM.read(baslat+i); //  EEPROM dan diziye okunabilen değerler atayın.
  }
}

////////////////////EEPROM a ID Ekleme///////////////////////////////////

void IDyazma(byte a[])
{
  if (!IDbul(a))     //biz eeprom a yazmadan önce önceden yazılıp yazılmadığını kontrol edin.
  {
    int sayi=EEPROM.read(0);  
    int baslat= (sayi*4)+6;
    sayi++;
    EEPROM.write(0,sayi);
    for(int j=0;j<4;j++)
    {
      EEPROM.write(baslat+j,a[j]);
    }
    basariliYazma();
    Serial.println(F("Basarili bir sekilde ID kaydi EEPROM'a eklendi.."));
    }
    else
    {
      hataliYazma();
      Serial.println(F("Basarisiz! Yanlıs ID veya kotu EEPROM. :-("));
    }
}


/////////////////////////EEPROM'dan ID Silme////////////////////////

void IDsilme(byte a[])
{
  if (!IDbul(a))       //Önceden EEPROM'dan silinen karta sahipmiyiz kontrol et.
  {
    hataliYazma();             //değilse
   Serial.println(F("Basarisiz! Yanlıs ID veya kotu EEPROM. :-("));
  }
  else
  {
    int sayi=EEPROM.read(0);
    int slot;
    int baslat;
    int dongu;
    int j;
    int count=EEPROM.read(0);  //Kart numarasını saklayan ilk EEPROM'un ilk byte'ını oku
    slot=IDbulSLOT(a);
    baslat=(slot*4)+2;
    dongu=((sayi-slot)*4);
    sayi--;                            //tek sayacı azaltma
    EEPROM.write(0,sayi);
    for(j=0;j<dongu;j++)
    {
      EEPROM.write(baslat+j,EEPROM.read(baslat+4+j));
    }
    for(int k=0;k<4;k++)
    {
      EEPROM.write(baslat+j+k,0);
    }
    basariliSilme();
    Serial.println(F("Basarili bir sekilde ID kaydi EEPROM'dan silinmistir.."));
  }
}


///////////////Byte'ların Kontrolü///////////////////

boolean kontrolIKI (byte a[],byte b[])
{
  if (a[0] != NULL)
  match=true;
  for (int k=0; k<4; k++)
  {
    if(a[k] != b[k])
    {
      match=false;
    }
    if(match)
    {
      return true;
    }
    else 
    {
      return false;
    }
  }
}

//////////////////////Boşluk Bulma////////////////////////////

int IDbulSLOT (byte find[] )
{
  int count = EEPROM.read(0);     //EEPROM ile ilk byte ı okuyacağız.
  for(int i=1; i<=count; i++)      //Döngüdeki her EEPROM girişi için
  {
    readID(i);                  //EEPROM daki ID yi okuyacak ve Storedcard[4] de saklayacağız.
    if (kontrolIKI(find, storedCard))  // Saklı Kartlar da olup olmadığının kontrolü.
    { //aynı ID'e sahip kart bulursa geçişe izin vericek.
      return i;   //kartın slot numarası
      break;       //aramayı durdurucak.
    }
  }
}

///////////////////////EEPROM'da ID Bulma//////////////////////

boolean IDbul (byte find[] )
{
  int count = EEPROM.read(0);    //EEPROM'daki ilk byte'ı oku
  for(int i=1; i <= count; i++)    //Önceden giriş yapılmış mı kontrolü.
  {
    readID(i);      
    if(kontrolIKI(find,storedCard) )
    {
      return true;
      break;
    }
    else
    {
      //değilse return false
    }
  }
  return false;
}


///////////////////Başarılı Şekilde EEPROM'a Yazma//////////////////

void basariliYazma() 
{
  digitalWrite(yellowLed, LED_OFF);   // sarı ledin sönük olduğundan emin olun.
  digitalWrite(redLed, LED_OFF);  // kırmızı ledin sönük olduğundan emin olun.
  digitalWrite(greenLed, LED_OFF);  // yeşil ledin sönük olduğundan emin olun.
  delay(200);
  digitalWrite(greenLed, LED_ON);   // yeşil ledin yanık olduğundan emin olun.
  delay(200);
  digitalWrite(greenLed, LED_OFF);  // yeşil ledin sönük olduğundan emin olun.
  delay(200);
  digitalWrite(greenLed, LED_ON);   // yeşil ledin yanık olduğundan emin olun.
  delay(200);
  digitalWrite(greenLed, LED_OFF);  // yeşil ledin sönük olduğundan emin olun.
  delay(200);
  digitalWrite(greenLed, LED_ON);   // yeşil ledin yanık olduğundan emin olun.
  delay(200);
}

/////////////////////EEPROM'a Yazma İşlemi Başarısız/////////////////

void hataliYazma() 
{
  digitalWrite(yellowLed, LED_OFF);   // sarı ledin sönük olduğundan emin olun.
  digitalWrite(redLed, LED_OFF);  // kırmızı ledin sönük olduğundan emin olun.
  digitalWrite(greenLed, LED_OFF);  // yeşil ledin sönük olduğundan emin olun.
  delay(200);
  digitalWrite(redLed, LED_ON);   // kırmızı ledin yanık olduğundan emin olun.
  delay(200);
  digitalWrite(redLed, LED_OFF);  // kırmızı ledin sönük olduğundan emin olun.
  delay(200);
  digitalWrite(redLed, LED_ON);   // kırmızı ledin yanık olduğundan emin olun.
  delay(200);
  digitalWrite(redLed, LED_OFF);  // kırmızı ledin sönük olduğundan emin olun.
  delay(200);
  digitalWrite(redLed, LED_ON);   // kırmızı ledin yanık olduğundan emin olun.
  delay(200);
}

////////////////////Silme İşlemi Başarılı///////////////////////////

void basariliSilme() 
{
  digitalWrite(yellowLed, LED_OFF);   // sarı ledin sönük olduğundan emin olun.
  digitalWrite(redLed, LED_OFF);  // kırmızı ledin sönük olduğundan emin olun.
  digitalWrite(greenLed, LED_OFF);  // yeşil ledin sönük olduğundan emin olun.
  delay(200);
  digitalWrite(yellowLed, LED_ON);  // sarı ledin yanık olduğundan emin olun.
  delay(200);
  digitalWrite(yellowLed, LED_OFF);   // sarı ledin sönük olduğundan emin olun.
  delay(200);
  digitalWrite(yellowLed, LED_ON);  // sarı ledin yanık olduğundan emin olun.
  delay(200);
  digitalWrite(yellowLed, LED_OFF);   // sarı ledin sönük olduğundan emin olun.
  delay(200);
  digitalWrite(yellowLed, LED_ON);  // sarı ledin yanık olduğundan emin olun.
  delay(200);
}

////////////////Master Kartın Doğruluğunun Tespiti///////////////////

boolean MasterDogrulama (byte test[])
{
  if(kontrolIKI(test,masterCard))
  return true;
  else
  return false;
  
}








