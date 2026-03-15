# AeroTrack-SDR: Sistem Dokumantasyonu

> Bu dokuman, AeroTrack-SDR projesindeki tum radar parametrelerini, sinyal isleme
> kavramlarini ve simulasyon akisini aciklar. Pulse-Doppler radar literaturune
> hakim olmanizi saglayacak bir referans olarak tasarlanmistir.

---

## Icindekiler

1. [Radar Nedir? Temel Calisma Prensibi](#1-radar-nedir-temel-calisma-prensibi)
2. [Pulse-Doppler Radar Mimarisi](#2-pulse-doppler-radar-mimarisi)
3. [I/Q (In-Phase / Quadrature) Veri Formati](#3-iq-in-phase--quadrature-veri-formati)
4. [LFM Chirp Darbesi](#4-lfm-chirp-darbesi)
5. [Radar Parametreleri Detayli Referans](#5-radar-parametreleri-detayli-referans)
   - 5.1 [RF Parametreleri](#51-rf-parametreleri)
   - 5.2 [Zamanlama Parametreleri](#52-zamanlama-parametreleri)
   - 5.3 [Ornekleme Parametreleri](#53-ornekleme-parametreleri)
   - 5.4 [Gurultu Parametreleri](#54-gurultu-parametreleri)
   - 5.5 [Hedef Parametreleri](#55-hedef-parametreleri)
6. [Turetilmis Parametreler ve Fiziksel Anlamlari](#6-turetilmis-parametreler-ve-fiziksel-anlamlari)
7. [Sinyal Isleme Zinciri](#7-sinyal-isleme-zinciri)
   - 7.1 [Chirp Uretimi](#71-chirp-uretimi)
   - 7.2 [Menzil Gecikmesi](#72-menzil-gecikmesi)
   - 7.3 [Doppler Frekans Kaymasi](#73-doppler-frekans-kaymasi)
   - 7.4 [Genlik Olcekleme (Radar Denklemi)](#74-genlik-olcekleme-radar-denklemi)
   - 7.5 [Darbe-Arasi Faz Ilerlemesi](#75-darbe-arasi-faz-ilerlemesi)
   - 7.6 [AWGN Gurultu Ekleme](#76-awgn-gurultu-ekleme)
8. [Range-Doppler Isleme](#8-range-doppler-isleme)
   - 8.1 [CPI Matrisi](#81-cpi-matrisi)
   - 8.2 [Range Compression (Matched Filter)](#82-range-compression-matched-filter)
   - 8.3 [Doppler Processing](#83-doppler-processing)
   - 8.4 [Range-Doppler Haritasi](#84-range-doppler-haritasi)
9. [FFT: Neden ve Nasil](#9-fft-neden-ve-nasil)
10. [Paketleme ve UDP Iletimi](#10-paketleme-ve-udp-iletimi)
11. [Simulasyon Akisi (Butunsel Gorunum)](#11-simulasyon-akisi-butunsel-gorunum)
12. [Sistem Mimarisi](#12-sistem-mimarisi)
13. [Parametre Etkilesim Tablosu](#13-parametre-etkilesim-tablosu)
14. [Literatur Sozlugu](#14-literatur-sozlugu)

---

## 1. Radar Nedir? Temel Calisma Prensibi

RADAR = **Ra**dio **D**etection **a**nd **R**anging (Radyo ile Tespit ve Mesafe Olcumu).

Temel calisma prensibi son derece basittir:

```
1. Verici elektromanyetik dalga yayar           (darbe gonderir)
2. Dalga hedefe carpar ve geri yansir            (eko)
3. Alici geri donen sinyali yakalar              (alim)
4. Gidis-donus suresi olculur                    (menzil hesabi)
5. Frekans degisimi olculur                      (hiz hesabi)
```

Bir el feneri dusunun: karanliktaki bir duvara isik tutuyorsunuz. Isigin gidip donme
suresi ne kadar uzunsa, duvar o kadar uzaktadir. Eger duvar size dogru hareket ediyorsa,
geri donen isigin rengi (frekansi) hafifce degisir — buna **Doppler etkisi** denir.
Ambulans siren sesinin yaklasirken tizlesmesi, uzaklasirken peslesmesi ile ayni fizik.

Radar da ayni seyi radyo dalgalariyla yapar. Tek fark: isik yerine mikrodalga frekansinda
(bu projede 10 GHz, yani dalga boyu 3 cm) elektromanyetik dalga kullanilir.

### Neden "Darbe" (Pulse) Gonderiyoruz?

Surekli dalga gondersek, gonderirken ayni anda almamiz gerekirdi — bu cok zordur ve
gonderici aliciyi sagirlastirir. Bunun yerine:

```
Gonder ─┤████├──────────────────┤████├──────────────────┤████├─── ...
Al    ──────────┤░░░░├──────────────────┤░░░░├──────────────── ...
         ^                        ^
      Darbe suresi (T)        PRI (darbe arasi)
```

Kisa bir darbe gonderip, sonra sessizce dinliyoruz. Gelen eko sinyalinden:
- **Ne zaman** geldi → menzil (range)
- **Frekansi ne kadar degisti** → hiz (velocity)

---

## 2. Pulse-Doppler Radar Mimarisi

AeroTrack-SDR bir **Pulse-Doppler** radardir. Bu, hem menzil hem hiz olcumu yapar.
Geleneksel pulse radarindan farki: **birden fazla darbeyi koherent (fazsal tutarli)
olarak isler**.

### CPI (Coherent Processing Interval)

Tek bir darbe ile hiz olcumu yapilamaz. Bunun icin bir **darbe grubu** gonderilir:

```
CPI (Coherent Processing Interval)
├── Darbe 0 ──┐
├── Darbe 1 ──┤
├── Darbe 2 ──┤  64 darbe bir CPI olusturur
├── ...       ┤  Her darbe ayni frekansta, ayni PRI aralikla
├── Darbe 62 ─┤
└── Darbe 63 ─┘
```

Bir CPI icindeki tum darbeler **koherent**tir — yani hepsinin baslangic fazlari
birbiriyle iliskilidir. Hareketli bir hedeften donen her darbenin fazi biraz daha kayar.
Bu faz kayma oruntusune bakarak hedefin hizini olceriz (Doppler isleme).

### Koherent Neden Onemli?

"Koherent" kelimesi literaturde cok gecer. Anlami: gonderdigimiz her darbenin fazi
birbiriyle **deterministik bir iliski** icinde. Rastgele degil.

Bunu bir muzik analojisiyle dusunun: Koherent olmayan radar, her darbede rastgele
notalar calan bir muzisyen gibidir — hiz olcemezsiniz. Koherent radar ise duzenli
bir melodi calar — hedef hareket ediyorsa melodide sistematik bir kayma olur, bu
kaymayi olcersiniz.

---

## 3. I/Q (In-Phase / Quadrature) Veri Formati

Radar alicisi sinyali dogrudan GHz frekansinda islemeye calismaz — bu pratik degildir.
Bunun yerine sinyal **baseband**'a (temel bant, 0 Hz cevresine) indirilir. Bu islem
sirasinda sinyal iki bilesene ayrilir:

```
                     cos(2π f_c t)
Alicidan ──────┬──────[X]──────► I (In-Phase, Eszfazli)
gelen sinyal   │
               │     sin(2π f_c t)
               └──────[X]──────► Q (Quadrature, Dik-fazli)
```

**I (In-Phase)**: Sinyalin cosinus bileseni — "gercek" kisim.
**Q (Quadrature)**: Sinyalin sinus bileseni — "sanal" kisim.

Neden iki bilesen? Cunku tek bir bilesen ile sinyalin **fazini** bilemezsiniz.
Iki bileseni bir arada kullanarak:
- **Genlik** = sqrt(I² + Q²)
- **Faz** = atan2(Q, I)

Bu ikili gosterim, matematikte **kompleks sayi** ile ifade edilir:

```
s[n] = I[n] + j·Q[n]     (j = sanal birim, elektrik muhendisliginde i yerine j kullanilir)
```

Bu projede her I/Q ornegi `std::complex<float>` tipinde saklanir: 4 byte I + 4 byte Q = 8 byte.

### Gercek Hayattaki Karsiligi

ASELSAN'da calistiginizda, FPGA veya DSP'den gelen veri akisi bu formattadir:
alternating I, Q, I, Q, ... degerleri. ADC (Analog-to-Digital Converter) iki kanalda
ornekler alir ve size I/Q ciftleri olarak teslim eder.

---

## 4. LFM Chirp Darbesi

Bu projede gonderilen darbe sade bir sinuzoid degil, **LFM (Linear Frequency Modulated)
chirp** darbesidir. "Chirp" kelimesi kus civiltisi anlamina gelir — cunku frekansi
zamanla artar, tipiyla ses yukselen bir kus civiltisi gibi.

### Neden Duz Bir Darbe Degil de Chirp?

**Temel ikilem**: Menzil cozunurlugu dar darbe ister (kisa T), algilama menzili ise
yuksek enerji ister (uzun T). Bu ikisi celistiir — kisa darbe az enerji tasir.

**Cozum**: Uzun ama frekans-moduleli bir darbe gonder. Alicida **matched filter**
(eslestirilmis filtre) ile bu uzun darbeyi sikistir. Sonuc: hem uzun darbenin
enerjisine hem kisa darbenin cozunurlugune sahip olursun.

```
Frekans
  ^
  │         ╱
  │       ╱         Frekans zamanla dogrusal artar
  │     ╱           f(t) = f_0 + μ·t
  │   ╱
  │ ╱               μ = chirp rate = B/T
  └───────────► Zaman
  0          T
    Darbe suresi
```

Matematiksel ifade (baseband):

```
s(t) = exp(j · π · μ · t²)

burada:
  μ  = B / T     (chirp rate, Hz/s)
  B  = bandwidth (bant genisligi)
  T  = darbe suresi
  t  = zaman (0'dan T'ye)
```

Bu projede: B = 5 MHz, T = 10 us → μ = 5×10⁶ / 10×10⁻⁶ = 5×10¹¹ Hz/s = 500 GHz/s.
Yani darbe suresi boyunca frekans 500 milyar Hz/saniye hizla artar.

### Gercek Hayattaki Karsiligi

Gercek bir radar vericisinde DDS (Direct Digital Synthesizer) veya AWG (Arbitrary
Waveform Generator) bu chirp sinyalini uretir. FPGA'de look-up table + NCO
(Numerically Controlled Oscillator) ile de uretilir. Aselsan REHIS'te calistiginiz
sistemlerde buyuk ihtimalle FPGA tabanli chirp uretimi goreceksiniz.

---

## 5. Radar Parametreleri Detayli Referans

### 5.1 RF Parametreleri

#### `carrier_frequency_hz` — Tasiyici Frekans
- **Varsayilan**: 10 GHz (10×10⁹ Hz)
- **Birim**: Hz
- **Koddaki yer**: `radar_config.hpp:17`
- **Anlamı**:

Radarın yayın yaptığı elektromanyetik dalganın frekansı. Bu projede **X-band**
(8-12 GHz) kullanılır.

Frekans bandları ve kullanım alanları:

```
Band     Frekans        Kullanım
───────────────────────────────────────────────────────
L-band   1-2 GHz        Hava trafik kontrolü, uzun menzil gözetleme
S-band   2-4 GHz        Havaalanı radarı, deniz radarı
C-band   4-8 GHz        Hava durumu radarı
X-band   8-12 GHz       Askeri radar, uydu haberleşme, SAR görüntüleme
Ku-band  12-18 GHz      Yüksek çözünürlüklü görüntüleme
Ka-band  26-40 GHz      Otomotiv radar (77 GHz yakını), kısa menzil
```

**Neden önemli**: Frekans, dalga boyunu belirler. Dalga boyu ise:
- Antenin fiziksel boyutunu (küçük λ → küçük anten)
- Atmosferik zayıflamayı (yüksek f → daha fazla zayıflama)
- Doppler hassasiyetini (küçük λ → aynı hız, daha büyük Doppler kayması)
- Hedef yansıma karakteristiğini belirler

**Trade-off**: Yüksek frekans → daha iyi açısal çözünürlük ve Doppler hassasiyeti,
ama daha fazla atmosferik kayıp ve daha kısa menzil.

#### `bandwidth_hz` — Chirp Bant Genişliği
- **Varsayılan**: 5 MHz (5×10⁶ Hz)
- **Birim**: Hz
- **Koddaki yer**: `radar_config.hpp:18`
- **Anlamı**:

LFM chirp darbesinin kapsadığı frekans aralığı. Darbe başlangıcında f₀ frekansında
başlar, sonunda f₀ + B frekansına ulaşır.

```
|←────── B = 5 MHz ──────→|
f₀                       f₀ + B
```

**Doğrudan etkisi**: Menzil çözünürlüğünü belirler.

```
ΔR = c / (2B) = 3×10⁸ / (2 × 5×10⁶) = 30 metre
```

5 MHz bant genişliği ile birbirine 30 metreden yakın iki hedefi ayırt edemezsiniz —
ikisi tek bir hedef gibi görünür.

**Trade-off**: Geniş bant → iyi çözünürlük, ama daha yüksek örnekleme hızı gerektirir
(Nyquist teoremine göre f_s ≥ 2B) ve daha fazla veri üretir.

#### `pulse_duration_s` — Darbe Süresi (T)
- **Varsayılan**: 10 µs (10×10⁻⁶ s)
- **Birim**: saniye
- **Koddaki yer**: `radar_config.hpp:19`
- **Anlamı**:

Tek bir chirp darbesinin gönderilme süresi. Bu süre boyunca verici aktiftir.

**Neden önemli**: Darbe enerjisi = ortalama güç × süre. Uzun darbe → daha fazla enerji
→ daha uzak hedefleri tespit edebilirsiniz. Ama aynı zamanda "kör menzil" artar:
darbe gönderilirken alıcı dinleyemez.

```
Kör menzil = c × T / 2 = 3×10⁸ × 10×10⁻⁶ / 2 = 1500 m
```

Bu, 1500 metreden yakın hedefleri göremeyeceğimiz anlamına gelir (darbe hâlâ
gönderilirken eko geri dönüyor).

**Chirp ile çözüm**: Matched filter sonrası etkin darbe süresi T değil, 1/B olur.
Yani çözünürlük T'ye değil B'ye bağlıdır. Bu pulse-compression'ın büyüsüdür.

**Zaman-Bant Genişliği Çarpımı (Time-Bandwidth Product)**:

```
TBP = B × T = 5×10⁶ × 10×10⁻⁶ = 50
```

TBP, chirp darbesinin "sıkıştırma kazancını" gösterir. 50 demek: matched filter
sonrası sinyal, sıkıştırılmamış haline göre ~17 dB (10·log₁₀(50)) daha güçlü görünür.
Literatürde buna **processing gain** denir.

### 5.2 Zamanlama Parametreleri

#### `prf_hz` — Pulse Repetition Frequency (Darbe Tekrarlama Frekansı)
- **Varsayılan**: 1000 Hz (saniyede 1000 darbe)
- **Birim**: Hz
- **Koddaki yer**: `radar_config.hpp:22`
- **Anlamı**:

Saniyede kaç darbe gönderildiği. PRI (Pulse Repetition Interval) = 1/PRF.

```
├──T──┤                    ├──T──┤                    ├──T──┤
│█████│────────────────────│█████│────────────────────│█████│────
│     │                    │     │                    │     │
←──────── PRI = 1 ms ────→
```

PRF = 1000 Hz → PRI = 1 ms → her 1 milisaniyede bir darbe gönderilir.

**Kritik trade-off** — PRF hem menzili hem hızı etkiler, ve ikisi çelişir:

```
Yüksek PRF → İyi hız ölçümü (belirsizlik yok), kötü menzil (aliasing)
Düşük PRF  → İyi menzil ölçümü, kötü hız (belirsizlik var)
```

Bu, radar tasarımının en temel ikilemidir. Gerçek sistemlerde bunu çözmek için
**staggered PRF** (değişken PRF) veya **medium PRF** kullanılır.

#### `num_pulses_per_cpi` — CPI Başına Darbe Sayısı
- **Varsayılan**: 64
- **Birim**: adet
- **Koddaki yer**: `radar_config.hpp:23`
- **Anlamı**:

Bir Coherent Processing Interval'da kaç darbe gönderildiği.

**Neden 64?** Doppler işleme, pulse boyutu üzerinde FFT gerektirir. 64 = 2⁶,
yani FFT için ideal boyut (radix-2 FFT sadece 2'nin kuvvetleri için çalışır).

**Etkisi**:
- Daha fazla darbe → daha iyi hız çözünürlüğü (dar Doppler filtre genişliği)
- Daha fazla darbe → daha uzun CPI süresi (64 × 1 ms = 64 ms)
- Daha uzun CPI → hızlı manevra yapan hedeflerde bulanıklık (hız CPI boyunca değişirse)

```
Hız çözünürlüğü = λ / (2 × N × PRI) = 0.03 / (2 × 64 × 0.001) = 0.234 m/s ≈ 0.84 km/h
```

Yani 64 darbe ile 0.84 km/h'lik hız farkını ayırt edebilirsiniz.

#### `num_samples_per_pulse` — Darbe Başına Örnek Sayısı
- **Varsayılan**: 256
- **Birim**: adet
- **Koddaki yer**: `radar_config.hpp:24`
- **Anlamı**:

Her darbe süresince kaç I/Q örneği alındığı. Alıcının ADC'si darbe boyunca bu kadar
örnek alır.

**Neden 256?** 256 = 2⁸, range FFT için ideal boyut. Ayrıca:

```
Örnekleme süresi = 256 / 25.6 MHz = 10 µs = darbe süresi
```

Tam olarak darbe süresini kapsıyor. Her range bin bir örnekleme noktasına karşılık gelir.
Range FFT sonrası 256 range bin elde edilir.

**Gerçek hayat notu**: Pratikte alıcı penceresi darbe süresinden daha uzun açılır
(guard interval dahil). Ama bu simülasyonda sadece darbe süresi örneklenir.

### 5.3 Örnekleme Parametreleri

#### `sampling_frequency_hz` — ADC Örnekleme Frekansı
- **Varsayılan**: 25.6 MHz (25.6×10⁶ Hz)
- **Birim**: Hz
- **Koddaki yer**: `radar_config.hpp:27`
- **Anlamı**:

Analog I/Q sinyalinin dijitale çevrilme hızı. Saniyede 25.6 milyon örnek alınır.

**Neden 25.6 MHz?** İki nedeni var:

1. **Nyquist kriteri**: Bant genişliği B = 5 MHz, örnekleme f_s = 25.6 MHz > 2B = 10 MHz. ✓
2. **Güzel bölünme**: 25.6 MHz × 10 µs = 256 örnek (tam sayı, 2'nin kuvveti). ✓

**Örnekleme periyodu**: dt = 1/f_s = 1/25.6×10⁶ ≈ 39.06 ns

Her 39 nanosaniyede bir I/Q çifti alınır. Bu, kodda `signal_generator.cpp:13`'teki
`dt_` değişkenidir.

**Gerçek hayat notu**: Gerçek radar ADC'leri genellikle 14-16 bit çözünürlüktedir
ve 100+ MHz'lerde örnekler. Bu simülasyonda float32 kullanılır (sonsuz bit derinliği
benzeri).

### 5.4 Gürültü Parametreleri

#### `snr_db` — Sinyal-Gürültü Oranı
- **Varsayılan**: 20 dB
- **Birim**: desibel (dB)
- **Koddaki yer**: `radar_config.hpp:30`
- **Anlamı**:

Alınan sinyalin gürültüye göre ne kadar güçlü olduğu.

```
SNR_dB = 10 × log₁₀(P_sinyal / P_gürültü)

20 dB → P_sinyal / P_gürültü = 100    (sinyal gürültüden 100 kat güçlü)
10 dB → oran = 10
 3 dB → oran = 2
 0 dB → oran = 1    (sinyal ve gürültü eşit — tespit çok zor)
-5 dB → oran = 0.32  (sinyal gürültünün altında — tek darbe ile tespitilemez)
```

**Neden dB kullanılır?** Radar sinyalleri çok büyük dinamik aralıkta değişir.
1 km'deki hedefin sinyali, 100 km'dekinden 10⁸ (100 milyon) kat daha güçlüdür.
dB ölçeğinde bu 80 dB'lik bir fark olur — çok daha yönetilebilir bir sayı.

**Pratikte**: 20 dB çok temiz bir sinyal demektir. Gerçek dünyada uzak/küçük hedefler
0-10 dB aralığında olur. Negatif SNR'lerde bile **koherent entegrasyon** ile tespit
mümkündür (CPI'daki tüm darbeleri toplayarak).

**Koherent entegrasyon kazancı**:

```
SNR_çıkış = SNR_giriş + 10·log₁₀(N)

N = 64 darbe → kazanç = 10·log₁₀(64) = 18 dB
Giriş SNR = 2 dB → Çıkış SNR = 20 dB (artık rahatlıkla tespit edilebilir)
```

Bu, pulse-Doppler radarın en büyük avantajıdır.

### 5.5 Hedef Parametreleri

#### `range_m` — Hedef Menzili
- **Varsayılan**: 5000 m
- **Birim**: metre
- **Anlamı**:

Radardan hedefe olan doğrusal mesafe. Sinyal gidip geri dönmek zorunda olduğundan,
toplam yol 2R'dir.

```
Gecikme = 2R / c = 2 × 5000 / 3×10⁸ = 33.33 µs
Örnekleme cinsinden = 33.33 µs × 25.6 MHz ≈ 853 örnek
```

Bu, 256 örneklik darbe penceremizin dışında! Simülasyonda gecikme modulo 256
alınır (periodic extension). Gerçek sistemlerde bu menzil, alıcı penceresinin
açılma zamanlamasıyla kontrol edilir.

#### `velocity_kmh` — Hedef Hızı
- **Varsayılan**: 300 km/h (83.33 m/s)
- **Birim**: km/h (kodda m/s'ye çevrilir: v_ms = v_kmh / 3.6)
- **Anlamı**:

Hedefin radara göre radyal hızı. Pozitif değer = radara yaklaşan hedef.

```
Doppler frekansı: f_d = 2v / λ = 2 × 83.33 / 0.03 = 5555.6 Hz ≈ 5.56 kHz
```

Bu Doppler frekansı, her darbe arasındaki faz değişimini belirler:

```
Darbe-arası faz = 2π × f_d × PRI = 2π × 5556 × 0.001 = 34.9 radyan
```

PRF = 1000 Hz ile max belirsiz hız = ±27 km/h. 300 km/h bu sınırı aştığından,
Doppler **aliasing** oluşur (hız, -27 ile +27 arasına katlanır). Bu simülasyonda
bu bilerek tolere edilir.

#### `rcs_m2` — Radar Kesit Alanı (RCS)
- **Varsayılan**: 10 m²
- **Birim**: m² (metrekare)
- **Anlamı**:

Hedefin radardan gelen dalgayı ne kadar etkili bir şekilde yansıttığının ölçüsü.
Fiziksel boyutla doğrudan ilgili değildir — şekle, malzemeye ve frekansa bağlıdır.

Tipik RCS değerleri:

```
Hedef                  RCS (m²)      dBsm
───────────────────────────────────────────
Kuş                    0.01          -20
Drone (küçük)          0.1           -10
İnsan                  1              0
Otomobil               10            +10
Küçük uçak             2-5           +3 ila +7
Savaş uçağı (normal)   3-10          +5 ila +10
Savaş uçağı (stealth)  0.001-0.01   -30 ila -20
Büyük gemi             10000         +40
```

**Kodda etkisi**: `signal_generator.cpp:100`

```cpp
amplitude = sqrt(RCS) / R²
```

Bu, radar denkleminin basitleştirilmiş halidir. RCS büyüdükçe sinyal güçlenir,
menzil arttıkça R⁴'le (güç olarak) veya R²'yle (genlik olarak) zayıflar.

#### `azimuth_deg` — Azimut Açısı
- **Varsayılan**: 0°
- **Birim**: derece
- **Anlamı**:

Hedefin radarın baktığı yöne göre yatay düzlemdeki açısı. 0° = tam önde.
Bu projede azimut doğrudan sinyal işlemeye katılmaz (tek kanal simülasyonu),
sadece PPI (Plan Position Indicator) görüntülemede kullanılır.

Gerçek sistemlerde azimut, faz dizisi (phased array) anten yönlendirmesi veya
mekanik dönen anten ile belirlenir.

---

## 6. Türetilmiş Parametreler ve Fiziksel Anlamları

Bu parametreler doğrudan girilmez, temel parametrelerden hesaplanır.
Kaynak: `radar_config.hpp:37-63`

### `wavelength()` — Dalga Boyu (λ)

```
λ = c / f = 3×10⁸ / 10×10⁹ = 0.03 m = 3 cm
```

Dalga boyu, radarın "göz çözünürlüğüdür". Kısa dalga boyu → daha keskin göz.
X-band (3 cm) askeri radarlar için idealdir: yeterince kısa dalga boyu (iyi
çözünürlük) ama atmosferde aşırı zayıflama yok.

### `chirp_rate()` — Chirp Hızı (μ)

```
μ = B / T = 5×10⁶ / 10×10⁻⁶ = 5×10¹¹ Hz/s
```

Frekansın zamana göre değişim hızı. Chirp'in "dikliği". Matched filter'ın tasarımında
doğrudan kullanılır.

### `pri_s()` — PRI (Pulse Repetition Interval)

```
PRI = 1 / PRF = 1 / 1000 = 0.001 s = 1 ms
```

İki ardışık darbe arasındaki süre. Bu süre zarfında:
- İlk ~10 µs: darbe gönderilir
- Kalan ~990 µs: ekoların gelmesi beklenir

### `max_unambiguous_range()` — Maksimum Belirsiz Olmayan Menzil

```
R_max = c × PRI / 2 = 3×10⁸ × 0.001 / 2 = 150 km
```

Bu menzilden daha uzaktaki hedeflerden gelen ekolar, bir sonraki darbe zamanına
denk gelir ve yanlış (daha yakın) menzilde görünür. Buna **range aliasing** denir.

```
Darbe 1 gönder → 150 km'den uzak hedefin ekosu → Darbe 2 zamanında gelir
                                                    → "sanki yakınmış" gibi görünür
```

### `max_unambiguous_velocity()` — Maksimum Belirsiz Olmayan Hız

```
V_max = λ × PRF / 4 = 0.03 × 1000 / 4 = 7.5 m/s = 27 km/h
```

Bu, Nyquist limitidir. Darbe arası faz değişimi π radyanı aşarsa, hız belirsiz
olur. ±27 km/h dışındaki hızlar yanlış hız değerine katlanır.

**Dikkat**: Bu değer PRF = 1000 Hz için çok düşüktür. Gerçek bir savaş uçağı
radarında PRF 10-300 kHz aralığında olur, V_max yüzlerce km/h'e çıkar.
Bu simülasyonda düşük PRF tercih edilmiştir çünkü menzil belirsizliği olmasın
diye (150 km sınır yeterli).

### `range_resolution()` — Menzil Çözünürlüğü (ΔR)

```
ΔR = c / (2B) = 3×10⁸ / (2 × 5×10⁶) = 30 m
```

Birbirine 30 metreden daha yakın iki hedef, menzil boyutunda ayrıştırılamaz.
Tek bir hedef gibi görünür.

**Önemli**: Bu çözünürlük darbe süresine değil, bant genişliğine bağlıdır.
Chirp + matched filter sayesinde, uzun darbe (yüksek enerji) ile kısa etkin
darbe (iyi çözünürlük) elde edilir.

### `velocity_resolution()` — Hız Çözünürlüğü (Δv)

```
Δv = λ / (2 × N × PRI) = 0.03 / (2 × 64 × 0.001) = 0.234 m/s ≈ 0.84 km/h
```

Birbirine 0.84 km/h'den daha yakın hızlardaki iki hedef, Doppler boyutunda
ayrıştırılamaz. N (darbe sayısı) arttıkça çözünürlük iyileşir.

---

## 7. Sinyal İşleme Zinciri

Bir darbenin (pulse) üretiminden iletilmesine kadar geçen adımlar:

```
[Chirp Üretimi] → [Menzil Gecikmesi] → [Doppler Kayması] → [Genlik Ölçekleme]
                                                                     │
                  [Paketleme + UDP] ← [Gürültü Ekleme] ← [Faz İlerlemesi]
```

### 7.1 Chirp Üretimi

**Kaynak**: `signal_generator.cpp:17-38`

Baseband LFM chirp üretilir. "Baseband" demek taşıyıcı frekans çıkarılmış halidir.

```cpp
for (n = 0 → 255):
    t = n × dt                          // dt = 1/25.6MHz ≈ 39.06 ns
    phase = π × μ × t²                  // μ = 5×10¹¹ Hz/s
    I[n] = cos(phase)
    Q[n] = sin(phase)
```

Bu formül, tam bir LFM chirp darbesinin baseband temsilidir. Frekans zamanla
doğrusal artar:

```
Anlık frekans = d(phase)/dt / (2π) = μ × t
t = 0    → f = 0
t = T/2  → f = B/2 = 2.5 MHz
t = T    → f = B = 5 MHz
```

### 7.2 Menzil Gecikmesi

**Kaynak**: `signal_generator.cpp:40-61`

Hedeften gelen eko, radara gecikmeyle döner:

```
τ = 2R / c     (gidiş + dönüş)
```

Dijital sinyalde bu gecikme, örnekleri kaydırarak modellenir:

```
delay_samples = round(τ × f_s) = round(2R × f_s / c)
delayed[n] = signal[n - delay_samples]     (sınır kontrolü ile)
```

Kayma miktarı menzile bağlıdır. Daha uzak hedef → daha fazla kayma → range FFT'de
daha yüksek bin'de peak görünür.

### 7.3 Doppler Frekans Kayması

**Kaynak**: `signal_generator.cpp:63-90`

Hareket eden hedeften dönen sinyalin frekansı kayar (Doppler etkisi):

```
f_d = 2v / λ
```

Çarpanın 2 olmasının nedeni: sinyal hedefe giderken bir kez, geri dönerken bir kez
daha sıkışır/genişler — çift yönlü Doppler.

Kodda bu, her örneğe faz döndürücü (phasor) uygulanarak yapılır:

```cpp
for (n = 0 → 255):
    t = n × dt
    phase = 2π × f_d × t
    shifted[n] = signal[n] × exp(j × phase)
```

`exp(j × phase)` ile çarpma, sinyalin frekansını f_d kadar kaydırır.
Yaklaşan hedef pozitif f_d üretir, uzaklaşan negatif.

### 7.4 Genlik Ölçekleme (Radar Denklemi)

**Kaynak**: `signal_generator.cpp:92-112`

Geri dönen sinyalin gücü, **radar denklemine** göre belirlenir:

```
        P_t × G² × λ² × σ
P_r = ─────────────────────
        (4π)³ × R⁴
```

Bu projede basitleştirilmiş genlik ölçekleme:

```
amplitude = sqrt(σ) / R²
```

**R⁴ kuralı**: Menzil 2 katına çıktığında alınan güç 16'ya (2⁴) bölünür.
Genlik olarak bu R² düşüşüne karşılık gelir.

Bu, radarın en temel fiziksel sınırlamasıdır: uzak hedefler çok zayıf sinyal döndürür.

### 7.5 Darbe-Arası Faz İlerlemesi

**Kaynak**: `target_simulator.cpp:40-51`

Bu adım, pulse-Doppler işlemenin kalbidir. Her ardışık darbede, hedefin hareketi
nedeniyle ek bir faz birikir:

```
φ(pulse) = 2π × f_d × pulse_index × PRI
```

Neden? Hedef her PRI süresinde `v × PRI` kadar hareket eder. Bu mesafe değişimi,
geri dönen sinyalde faz değişimine neden olur:

```
Darbe 0:  faz =     0    radyan
Darbe 1:  faz =   34.9   radyan   (≡ faz mod 2π)
Darbe 2:  faz =   69.8   radyan
...
Darbe 63: faz = 2198.7   radyan
```

Bu düzenli faz ilerlemesi, Doppler FFT ile ölçülür. Faz ilerlemesinin hızı = Doppler
frekansı = hedefin hızı.

**Kodda**: Her hedefin pulse bazındaki ekosuna `exp(j × φ)` ile çarpılır.

### 7.6 AWGN Gürültü Ekleme

**Kaynak**: `noise_generator.cpp:19-53`

Gerçek bir alıcıda her zaman gürültü vardır: termal gürültü (elektronik bileşenlerin
ısısından), galaktik gürültü, vb. Bu simülasyonda **AWGN (Additive White Gaussian Noise)**
modeli kullanılır:

- **Additive**: Sinyale toplanarak eklenir
- **White**: Tüm frekanslar eşit güçte (beyaz ışık gibi)
- **Gaussian**: Genlik dağılımı normal (Gauss) dağılımına uyar

```
noisy[n] = signal[n] + noise[n]

noise[n] = N(0, σ_I) + j × N(0, σ_Q)

σ = sqrt(P_noise / 2)
P_noise = P_signal / 10^(SNR_dB/10)
```

Her I ve Q bileşeni için bağımsız Gauss gürültü üretilir. σ değeri SNR'ye göre
ayarlanır: düşük SNR → büyük σ → daha fazla gürültü.

---

## 8. Range-Doppler İşleme

Bu, bu projenin sinyal işleme açısından en kritik bölümüdür. Tüm darbeleri
birleştirerek hem menzil hem hız bilgisi çıkarılır.

**Kaynak**: `range_doppler_processor.cpp`

### 8.1 CPI Matrisi

Bir CPI'daki tüm darbeler 2 boyutlu bir matrise dizilir:

```
              Sample 0    Sample 1    ...    Sample 255
            ┌──────────┬──────────┬────────┬──────────┐
Darbe 0     │ s(0,0)   │ s(0,1)   │  ...   │ s(0,255) │  → Fast-time (menzil boyutu)
Darbe 1     │ s(1,0)   │ s(1,1)   │  ...   │ s(1,255) │
...         │   ...    │   ...    │  ...   │   ...    │
Darbe 63    │ s(63,0)  │ s(63,1)  │  ...   │ s(63,255)│
            └──────────┴──────────┴────────┴──────────┘
                │
                ▼
           Slow-time (Doppler/hız boyutu)
```

**Fast-time** (yatay): Bir darbe içindeki örnekler. Menzil bilgisi burada.
**Slow-time** (dikey): Ardışık darbelerin aynı sample noktası. Hız bilgisi burada.

### 8.2 Range Compression (Matched Filter)

**Amaç**: Chirp darbesini sıkıştırarak menzil çözünürlüğünü elde etmek.

Matched filter, gönderilen chirp'in **conjugate** (eşlenik) ile korelasyondur.
Frekans domaininde:

```
1. Her darbe satırı için FFT al                        → S(f)
2. Referans chirp'in FFT'sinin conjugate'ını al         → H*(f)
3. Frekans domaininde çarp                              → S(f) × H*(f)
4. Sonucu IFFT ile zamana geri dön                      → sıkıştırılmış sinyal
```

```
Sıkıştırma öncesi:
───────╱╱╱╱╱╱╱╱╱──────────────   (uzun, yayılmış chirp ekosu)

Sıkıştırma sonrası:
─────────────█─────────────────   (keskin peak, menzili gösterir)
             ↑
          Hedef burada
```

Bu işlemin büyüsü: uzun chirp enerjisinin tamamı tek bir peak'e yoğunlaşır.
Peak'in genişliği ~ 1/B'ye eşittir, yani menzil çözünürlüğü bant genişliğine bağlıdır.

**Kodda** (`range_doppler_processor.cpp:58-83`):

```cpp
// Referans chirp FFT
fft(ref_fft);
conjugate(ref_fft);    // H*(f)

// Her darbe için
for (pulse p):
    fft(pulse_data);                    // S(f)
    pulse_data *= ref_fft;              // S(f) × H*(f)
    ifft(pulse_data);                   // → range compressed
```

### 8.3 Doppler Processing

**Amaç**: Her menzil noktasındaki hız bilgisini çıkarmak.

Range compression sonrası, her menzil noktası (range bin) için ardışık darbelerdeki
faz ilerlemesine bakarız. Bu, o range bin'deki hedefin Doppler frekansını verir.

```
Range bin r için:
    Darbe 0: range_compressed[0][r]  → karmaşık değer
    Darbe 1: range_compressed[1][r]  → karmaşık değer (faz biraz kaydi)
    ...
    Darbe 63: range_compressed[63][r] → karmaşık değer (faz çok kaydi)

    Bu 64 değere FFT uygula → Doppler spektrumu
```

FFT çıkışında peak'in konumu, o range bin'deki hedefin Doppler frekansını
(dolayısıyla hızını) verir.

**FFT Shift**: FFT çıkışı [0, f_max, ..., -f_max, -1] sırasındadır. Bunu
[-f_max, ..., 0, ..., f_max] sırasına getirmek için FFT shift yapılır (negatif
hızlar solda, pozitif sağda).

**Kodda** (`range_doppler_processor.cpp:85-109`):

```cpp
for (range_bin r):
    doppler_col = range_compressed[:][r]   // tüm darbelerdeki r'inci sample
    fft(doppler_col)                        // Doppler FFT
    fftshift(doppler_col)                   // 0-freq'i ortala
    magnitude_dB(doppler_col) → rd_map[r]  // dB dönüşümü
```

### 8.4 Range-Doppler Haritası

Sonuç, 2 boyutlu bir haritadır:

```
           ← Hız (km/h) →
         -27  -13   0   +13  +27
        ┌────┬────┬────┬────┬────┐
  0 m   │    │    │    │    │    │   ← Gürültü tabanı (~-20 dB)
  30 m  │    │    │    │    │    │
  ...   │    │    │    │    │    │
 5000 m │    │    │ ██ │    │    │   ← Hedef peak! (+30 dB)
  ...   │    │    │    │    │    │
150 km  │    │    │    │    │    │
        └────┴────┴────┴────┴────┘
```

Her piksel bir (menzil, hız) hücresini temsil eder. Parlak noktalar hedefleri gösterir.
Bu harita, radarın "gördüğü" dünyadır — operatörün veya otomatik tespit algoritmasının
baktığı ilk çıktıdır.

**CFAR (Constant False Alarm Rate)**: Gerçek sistemlerde bu haritaya CFAR algoritması
uygulanır — gürültü tabanını otomatik tahmin edip, eşiğin üzerindeki peak'leri "hedef"
olarak işaretler. Bu projede CFAR henüz implemente edilmemiştir.

---

## 9. FFT: Neden ve Nasıl

FFT (Fast Fourier Transform), bu projede 3 yerde kullanılır:

| Kullanım | Boyut | Amaç |
|----------|-------|------|
| Range compression FFT | 256 nokta | Chirp sıkıştırma (menzil) |
| Range compression IFFT | 256 nokta | Frekans → zaman dönüşü |
| Doppler FFT | 64 nokta | Hız ölçümü |

### FFT Ne Yapar?

Zaman domainindeki sinyali frekans bileşenlerine ayırır.

```
Zaman domaini:  ~~~~∿∿∿~~~~    (karmaşık dalga)
       ↓ FFT
Frekans domaini: ─▃─█─▃──      (hangi frekanslar var, ne kadar güçlü)
```

### Neden FFT, Neden DFT Değil?

DFT (Discrete Fourier Transform) O(N²) işlem gerektirir.
FFT aynı sonucu O(N·log₂N) işlemde verir.

```
N = 256:
  DFT:  256² = 65,536 çarpma
  FFT:  256 × 8 = 2,048 çarpma   (32× daha hızlı)
```

Bu projede Cooley-Tukey radix-2 DIT (Decimation In Time) FFT kullanılır.
Bu, en klasik FFT algoritmasıdır. Sadece 2'nin kuvveti boyutlarda çalışır
(256 = 2⁸, 64 = 2⁶ — ikisi de uygun).

### IFFT Nasıl Yapılır?

IFFT = conjugate → FFT → conjugate → 1/N ile böl. Bu projede tam olarak bu yöntem
kullanılır (`range_doppler_processor.cpp:43-52`).

### FPGA'de FFT

Gerçek radar sistemlerinde FFT, FPGA üzerinde pipeline mimarisinde çalışır.
Xilinx/AMD FFT IP core'u veya custom HDL ile implemente edilir. Her clock
cycle'da bir örnek girip, birkaç yüz cycle latency sonra sonuç çıkar.

---

## 10. Paketleme ve UDP İletimi

**Kaynak**: `packet_framer.cpp`, `udp_transmitter.cpp`

Üretilen I/Q verileri, gerçek bir radar alıcısına (bu projede ZCU102 FPGA kartı)
UDP üzerinden gönderilir.

### Paket Yapısı

```
┌──────────────────────────────────┐
│         PacketHeader (24 byte)    │
├──────────────────────────────────┤
│ sync_word    │ 4 byte │ 0xAE50AE50 (senkronizasyon)          │
│ packet_id    │ 4 byte │ Sıralı paket numarası                │
│ pulse_index  │ 4 byte │ CPI içindeki darbe indexi (0-63)     │
│ sample_count │ 4 byte │ I/Q örnek sayısı (256)               │
│ cpi_index    │ 4 byte │ CPI numarası                         │
│ flags        │ 4 byte │ CPI_START (0x01), CPI_END (0x02)     │
├──────────────────────────────────┤
│         I/Q Data (2048 byte)      │
├──────────────────────────────────┤
│ I[0] Q[0]   │ 8 byte │ float32 + float32                    │
│ I[1] Q[1]   │ 8 byte │                                      │
│ ...          │        │                                      │
│ I[255] Q[255]│ 8 byte │                                      │
└──────────────────────────────────┘
Toplam: 24 + 256×8 = 2072 byte/paket
```

### Neden UDP?

- **Düşük gecikme**: TCP'nin handshake, ACK ve yeniden gönderim mekanizmaları
  gerçek zamanlı veri akışında gecikme yaratır.
- **Paket kaybı tolere edilebilir**: Bir darbe kaybedilirse sonraki CPI'da
  telafi edilir. Kayıp darbelerin yeniden gönderilmesi mantıksızdır.
- **Basitlik**: Embedded sistemlerde (FPGA/DSP) UDP stack'i çok daha basittir.

### Veri Hızı

```
2072 byte × 1000 darbe/s = 2.072 MB/s
```

Bir CPI'daki tüm veri: 2072 × 64 = 132,608 byte ≈ 130 KB.

---

## 11. Simülasyon Akışı (Bütünsel Görünüm)

Aşağıda bir CPI'nın baştan sona işlenişi gösterilmiştir:

```
┌─────────────────────────────────────────────────────────────────────┐
│                         CPI DÖNGÜSÜ                                  │
│                                                                      │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                    DARBE DÖNGÜSÜ (×64)                       │    │
│  │                                                               │    │
│  │  1. Chirp üret           s_bb(t) = exp(jπμt²)               │    │
│  │           │                                                   │    │
│  │  2. Her hedef için:                                          │    │
│  │     ├── Menzil gecikmesi    τ = 2R/c → sample shift          │    │
│  │     ├── Doppler kayması     f_d = 2v/λ → faz döndürme       │    │
│  │     ├── Genlik ölçekleme    A = √σ / R²                      │    │
│  │     └── Darbe-arası faz     φ = 2πf_d × pulse × PRI         │    │
│  │           │                                                   │    │
│  │  3. Tüm hedeflerin ekoları toplanır                          │    │
│  │           │                                                   │    │
│  │  4. AWGN gürültü eklenir                                     │    │
│  │           │                                                   │    │
│  │  5. I/Q verisi CPI matrisine eklenir                         │    │
│  │           │                                                   │    │
│  │  6. [Opsiyonel] UDP paketi oluştur ve gönder                 │    │
│  │                                                               │    │
│  └───────────────────────────────────────────────────────────────┘    │
│                                                                      │
│  CPI tamamlandı:                                                     │
│                                                                      │
│  7. Hedef pozisyonlarını güncelle:  R -= v × CPI_süresi            │
│                                                                      │
│  8. Range-Doppler işleme:                                           │
│     ├── Her darbe → FFT → matched filter → IFFT (range compression) │
│     └── Her range bin → darbe-arası FFT → dB (Doppler processing)   │
│                                                                      │
│  9. Sonuçları paylaşımlı belleğe yaz:                               │
│     ├── latest_iq_waveform (son darbenin I/Q'su)                    │
│     ├── range_doppler_map (256×64 float, dB)                        │
│     └── target_history (hedef pozisyonları snapshot)                │
│                                                                      │
│  10. 10ms bekle (CPU throttling) → sonraki CPI'ya geç              │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### Zamanlama (Varsayılan Parametrelerle)

```
Bir darbe üretimi          :  ~sinyaller işlem süresi (CPU'ya bağlı)
Bir CPI (64 darbe)         :  ~birkaç ms (simülasyonda gerçek zamanlı bekleme yok)
Range-Doppler hesaplama    :  ~birkaç ms (256×64 FFT/IFFT)
Toplam bir CPI cycle       :  ~10-15 ms (+ 10ms sleep)
Gerçek radar zamanlaması   :  64 ms (64 darbe × 1 ms PRI)
```

Simülasyon gerçek zamanlıdan ~5× hızlı çalışır. 10ms sleep olmasa çok daha hızlı
olur ama CPU'yu %100 kullanır.

---

## 12. Sistem Mimarisi

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           AeroTrack-SDR                                  │
│                                                                          │
│  ┌──────────────────────┐    ┌──────────────────────────────────────┐   │
│  │   Next.js Frontend    │    │        C++ REST API Backend          │   │
│  │   (localhost:3000)    │    │        (localhost:8080)               │   │
│  │                       │    │                                      │   │
│  │  ┌─────────────────┐ │    │  ┌──────────────┐  ┌──────────────┐ │   │
│  │  │  Dashboard Page  │ │    │  │  ApiServer    │  │  Simulation  │ │   │
│  │  │                  │ │    │  │  (13 endpoint)│  │  Engine      │ │   │
│  │  │  ┌────────────┐ │ │    │  │               │  │  (thread)    │ │   │
│  │  │  │ RD Heatmap │ │◄├────├──┤ GET /api/data/│  │              │ │   │
│  │  │  │ IQ Waveform│ │ │    │  │   range-doppl │  │ ┌──────────┐│ │   │
│  │  │  │ PPI Scope  │ │ │    │  │   waveform    │  │ │ aerotrack││ │   │
│  │  │  │ Tracker    │ │ │    │  │   target-hist │  │ │ _core.a  ││ │   │
│  │  │  │ Spectrum   │ │ │    │  │               │  │ │          ││ │   │
│  │  │  │ Stats      │ │ │    │  │ PUT /api/     │  │ │SignalGen ││ │   │
│  │  │  └────────────┘ │ │    │  │   config      │  │ │TargetSim││ │   │
│  │  │                  │ │    │  │               │  │ │NoiseGen ││ │   │
│  │  │  ┌────────────┐ │ │    │  │ POST /api/    │  │ │PacketFrm││ │   │
│  │  │  │ Sidebar    │ │►├────├──┤   simulation/ │  │ │RDProcess││ │   │
│  │  │  │ Config     │ │ │    │  │   start|stop  │  │ └──────────┘│ │   │
│  │  │  │ Targets    │ │ │    │  │               │  │              │ │   │
│  │  │  │ Controls   │ │ │    │  │ POST/DELETE   │  │  SimData     │ │   │
│  │  │  └────────────┘ │ │    │  │   /api/targets│  │  (mutex)     │ │   │
│  │  └─────────────────┘ │    │  └──────────────┘  └──────────────┘ │   │
│  │                       │    │                                      │   │
│  │  next.config.ts:      │    │                                      │   │
│  │  /api/* → :8080/api/* │    │                                      │   │
│  └──────────────────────┘    └──────────────────────────────────────┘   │
│                                                                          │
│  ┌──────────────────────┐    ┌──────────────────────────────────────┐   │
│  │  CLI Transmitter      │    │  ZCU102 Receiver (opsiyonel)         │   │
│  │  (standalone binary)  │───►│  UDP :5000                           │   │
│  │  UDP gönderim modu    │    │  FPGA sinyal işleme                  │   │
│  └──────────────────────┘    └──────────────────────────────────────┘   │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### Veri Akışı

```
Kullanıcı (Browser)
    │
    ├── Config değiştir ──► PUT /api/config ──► SimulationData.config güncelle
    ├── Hedef ekle ────────► POST /api/targets ──► SimulationData.targets güncelle
    ├── Start tıkla ───────► POST /api/simulation/start ──► SimulationEngine.start()
    │                                                            │
    │                                                     Yeni thread başlat
    │                                                            │
    │                                                     CPI döngüsü çalışır
    │                                                            │
    ├── 300ms'de bir poll ─► GET /api/data/* ──► SimulationData'dan oku
    │                                                            │
    └── Stop tıkla ────────► POST /api/simulation/stop ──► stop_requested = true
                                                            Thread biter
```

### Thread Safety

SimulationEngine ayrı bir thread'de çalışır. API istekleri ana thread'den gelir.
`SimulationData::mtx` (std::mutex) ile korunur. Her iki taraf da veri okuma/yazma
öncesinde mutex lock alır.

---

## 13. Parametre Etkileşim Tablosu

Bir parametreyi değiştirdiğinizde nelerin etkilendiğini gösteren hızlı referans:

```
Parametre                │ Artırınca ne olur
─────────────────────────┼──────────────────────────────────────────────
carrier_frequency_hz ↑   │ λ ↓, Doppler hassasiyeti ↑, V_max ↓
bandwidth_hz ↑           │ ΔR ↓ (iyi), f_s gereksinimi ↑, veri hızı ↑
pulse_duration_s ↑       │ Enerji ↑ (iyi), kör menzil ↑, TBP ↑
prf_hz ↑                 │ V_max ↑ (iyi), R_max ↓ (kötü)
num_pulses_per_cpi ↑     │ Δv ↓ (iyi), CPI süresi ↑, işlem yükü ↑
num_samples_per_pulse ↑  │ Range bin sayısı ↑, paket boyutu ↑, veri ↑
sampling_frequency_hz ↑  │ Bant genişliği kapasitesi ↑, veri hızı ↑
snr_db ↑                 │ Daha temiz sinyal, daha kolay tespit
range_m ↑                │ Gecikme ↑, sinyal gücü ↓ (R⁴ kuralı)
velocity_kmh ↑           │ f_d ↑, Doppler kayma ↑
rcs_m2 ↑                 │ Daha güçlü eko (daha kolay tespit)
```

### Kritik Trade-off'lar

```
1. Menzil vs Hız Belirsizliği:
   R_max × V_max = c × λ / 8
   → Birini artırmak için diğerini feda etmelisin (PRF üzerinden)

2. Menzil Çözünürlüğü vs Veri Hızı:
   ΔR ↓  →  B ↑  →  f_s ↑  →  veri hızı ↑
   → Daha iyi çözünürlük = daha fazla bant genişliği = daha fazla veri

3. Hız Çözünürlüğü vs Güncelleme Hızı:
   Δv ↓  →  N ↑  →  CPI süresi ↑  →  güncelleme hızı ↓
   → Daha iyi hız çözünürlüğü = daha yavaş günceleme

4. Tespit Menzili vs Kör Menzil:
   Uzun darbe → daha fazla enerji → daha uzak tespit
   Uzun darbe → daha büyük kör menzil → yakın hedefleri kaçır
```

---

## 14. Literatür Sözlüğü

Bu projede ve radar literatüründe sıkça karşılaşacağınız terimler:

| Terim | İngilizce | Açıklama |
|-------|-----------|----------|
| ADC | Analog-to-Digital Converter | Analog sinyali dijitale çevirir |
| AWGN | Additive White Gaussian Noise | Beyaz Gauss gürültüsü modeli |
| Baseband | Baseband | Taşıyıcı frekans çıkarılmış sinyal (0 Hz civarı) |
| Chirp | Chirp | Frekansı zamanla değişen darbe sinyali |
| CFAR | Constant False Alarm Rate | Adaptif eşikleme ile hedef tespit algoritması |
| Clutter | Clutter | İstenmeyen yansımalar (yer, deniz, yağmur) |
| Coherent | Koherent | Fazsal tutarlılık; darbeler arası faz ilişkisi korunur |
| CPI | Coherent Processing Interval | Koherent olarak işlenen darbe grubu |
| DAC | Digital-to-Analog Converter | Dijital sinyali analoga çevirir (vericide) |
| DDS | Direct Digital Synthesizer | Dijital olarak dalga formu üreten devre |
| Doppler | Doppler | Hareket nedeniyle oluşan frekans kayması |
| DSP | Digital Signal Processor | Sinyal işlemeye özel işlemci |
| Dwell time | Bekleme süresi | Antenin bir yöne baktığı toplam süre |
| ECM | Electronic Counter-Measures | Elektronik karşı önlemler (jamming vb.) |
| ECCM | Electronic Counter-Counter-Measures | Karşı önlemlere karşı önlemler |
| EIRP | Effective Isotropic Radiated Power | Eşdeğer yayın gücü |
| Fast-time | Hızlı zaman | Bir darbe içindeki zaman ekseni (menzil) |
| FFT | Fast Fourier Transform | Hızlı Fourier Dönüşümü |
| FPGA | Field Programmable Gate Array | Programlanabilir mantık devresi |
| IFFT | Inverse FFT | Ters FFT (frekans → zaman) |
| IF | Intermediate Frequency | Ara frekans |
| I/Q | In-Phase / Quadrature | Sinyalin iki ortogonal bileşeni |
| LFM | Linear Frequency Modulation | Doğrusal frekans modülasyonu (chirp) |
| LNA | Low Noise Amplifier | Düşük gürültülü yükselteç (alıcı girişi) |
| Matched Filter | Eşleştirilmiş Filtre | Gönderilen sinyale göre optimal filtre |
| MTI | Moving Target Indication | Hareketli hedef gösterimi (clutter bastırma) |
| NCO | Numerically Controlled Oscillator | Dijital osilatör |
| Nyquist | Nyquist | f_s ≥ 2×f_max örnekleme teoremi |
| PD | Pulse-Doppler | Darbe-Doppler radar tipi |
| Phased Array | Faz Dizisi | Elektronik olarak yönlendirilen anten |
| PPI | Plan Position Indicator | Polar koordinatlı radar ekranı |
| PRF | Pulse Repetition Frequency | Darbe tekrarlama frekansı |
| PRI | Pulse Repetition Interval | Darbe tekrarlama aralığı = 1/PRF |
| Processing Gain | İşleme Kazancı | Chirp sıkıştırma + koherent entegrasyondan kazanç |
| Pulse Compression | Darbe Sıkıştırma | Uzun darbeyi kısa peak'e dönüştürme |
| Range bin | Menzil hücresi | Çözünürlük hücresinin menzil boyutu |
| Range gate | Menzil kapısı | Belirli bir menzil aralığını seçen zaman penceresi |
| RCS | Radar Cross Section | Radar kesit alanı |
| RD Map | Range-Doppler Map | Menzil-Doppler haritası |
| SAR | Synthetic Aperture Radar | Sentetik açıklıklı radar (görüntüleme) |
| SDR | Software Defined Radio | Yazılım tanımlı radyo |
| Sidelobe | Yan lob | Ana lobun yanındaki istenmeyen yayın/alım |
| Slow-time | Yavaş zaman | Darbe-arası zaman ekseni (Doppler/hız) |
| SNR | Signal-to-Noise Ratio | Sinyal-gürültü oranı |
| STAP | Space-Time Adaptive Processing | Uzay-zaman adaptif işleme |
| TBP | Time-Bandwidth Product | Zaman-bant genişliği çarpımı |
| Waveform | Dalga formu | Gönderilen sinyal şekli |

---

## Kaynaklar ve İleri Okuma

Bu konularda derinleşmek istiyorsanız:

1. **Skolnik, "Introduction to Radar Systems"** — Radar mühendisliğinin kutsal kitabı.
   Her kavram burada detaylıca açıklanır.
2. **Richards, "Fundamentals of Radar Signal Processing"** — Sinyal işleme odaklı.
   FFT, matched filter, CFAR konularında çok detaylı.
3. **Mahafza, "Radar Systems Analysis and Design Using MATLAB"** — Pratik, MATLAB
   örnekleriyle dolu. Simülasyon yazmak isteyenler için ideal.

---

*Bu dokuman AeroTrack-SDR projesi icin olusturulmustur.*
*Son guncelleme: 2026-03-15*
