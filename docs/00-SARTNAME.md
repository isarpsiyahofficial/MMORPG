# MMORPG — KO 1.298 Mobil Dönüşüm Şartnamesi

**Belge durumu:** BAĞLAYICI / Faz 0–1  
**Hedef platform:** Android APK / Unity 6 tabanlı istemci  
**Referans davranış:** Knight Online 1.298/1.299  
**Ana teknik referans:** `Open-KO/KnightOnline` commit `7d6cf81093e142c928c2ac9510512b2b182178b5`  
**Ana kural:** İlk çalışan baseline doğrulanmadan hiçbir görünür rework yapılmayacaktır.

---

## 1. Projenin amacı ve değişmez kabul kriterleri

- **REQ-001** İlk hedef yeni oyun tasarlamak değil, KO 1.298/1.299 istemci deneyimini Android üzerinde ayağa kaldırmaktır.
- **REQ-002** İlk baseline sürümde karakterler, item görünümleri, animasyonlar, haritalar, UI düzeni, skill ikonları, skill efektleri, sesler ve temel kamera davranışı mümkün olan en yüksek sadakatle korunacaktır.
- **REQ-003** Rework, yeniden adlandırma, yeni sanat yönetimi, yeni karakter/item/harita tasarımı yalnız parity baseline başarıyla doğrulandıktan sonra ayrı fazda yapılacaktır.
- **REQ-004** Port sırasında bir özelliğin davranışı belirsizse tahmin yürütülmeyecek; OpenKO kaynak kodu, dosya formatı, orijinal asset veya dönemin doğrulanabilir kullanıcı dokümantasyonu incelenecektir.
- **REQ-005** OpenKO kaynak kodunda görülen 1.298 davranışı, aksi kanıtlanmadıkça canonical davranış kabul edilecektir.
- **REQ-006** Kaynak referans commit sabit tutulacak; ileride referans commit değiştirilecekse ayrı migration kaydı tutulacaktır.
- **REQ-007** Orijinal asset üzerinde görünür optimizasyon değişikliği yapılması gerekiyorsa kaynak asset korunacak; Android türevi ayrı generated artifact olacaktır.
- **REQ-008** Kaynak assetlerin hash manifesti üretilecek ve orijinal dosyaların dönüşüm sırasında değişmediği doğrulanacaktır.
- **REQ-009** Generated Unity varlıkları kaynak dosyaların yerine geçmeyecek; yeniden üretilebilir cache/artifact olarak kabul edilecektir.
- **REQ-010** Her dönüştürücü deterministik çalışmalı; aynı input aynı tool sürümünde aynı logical output'u üretmelidir.
- **REQ-011** Dönüşüm hatası sessizce atlanmayacak; dosya adı, format, offset/record ve hata nedeni raporlanacaktır.
- **REQ-012** Bilinmeyen format alanları sıfır/varsayılan değerle körlemesine değiştirilmemelidir; korunabiliyorsa raw metadata olarak taşınmalıdır.
- **REQ-013** İlk APK'da PC'ye özel login ve server selection akışı kullanıcıya gösterilmeyecektir.
- **REQ-014** İlk APK açılışta doğrudan orijinal karakter oluşturma prosedürünün mobil karşılığına girecektir.
- **REQ-015** İlk APK testleri internet/server olmadan tamamlanabilmelidir.
- **REQ-016** Offline mod kalıcı ürün mimarisi olarak dayatılmayacak; online backend sonradan bağlanabilecek adapter sınırı korunacaktır.
- **REQ-017** SQL Server, MSSQL, ODBC, stored procedure veya D1 çalışma zamanı zorunluluğu bulunmayacaktır.
- **REQ-018** SQL'den gelen statik oyun verileri canonical SQL'siz veri paketlerine dönüştürülecektir.
- **REQ-019** Dinamik oyuncu verisi için storage interface kullanılacak; Faz 0'da local/offline implementation, sonraki fazda online implementation bağlanabilecektir.
- **REQ-020** UI, combat, item, skill ve world kodu storage motorunun SQL olup olmadığını bilmeyecektir.

## 2. Referans ve hak takibi

- **REQ-021** Kod referansı ile asset referansı ayrı provenance kaydında tutulacaktır.
- **REQ-022** OpenKO kod lisansı ile Knight Online'a ait üçüncü taraf/orijinal assetlerin kullanım hakkının aynı şey olmadığı kabul edilecektir.
- **REQ-023** Public/commercial dağıtım öncesinde her üçüncü taraf asset ailesi için lisans/izin durumu ayrıca gözden geçirilecektir.
- **REQ-024** Rework fazında isim değiştirmek tek başına özgünleştirme kabul edilmeyecektir.
- **REQ-025** Kaynak assetlerin hangi dosyadan hangi generated asset'e dönüştüğü provenance manifestinde izlenebilir olacaktır.
- **REQ-026** İnternetten bulunan rastgele client packleri canonical source ilan edilmeyecektir.
- **REQ-027** Aynı assetin birden fazla sürümü varsa referans sürüm açıkça seçilecek ve hash ile kilitlenecektir.
- **REQ-028** Kaynak depo veya asset paketindeki eksik dosyalar otomatik olarak başka versiyonla doldurulmayacaktır.
- **REQ-029** Eksik assetler blocker listesine yazılacak ve sürüm karışması engellenecektir.
- **REQ-030** Araştırma kaynakları `docs/research/` altında URL, erişim tarihi ve kullanım amacıyla kayıt altına alınacaktır.

## 3. Repository, build ve yeniden üretilebilirlik

- **REQ-031** `main` doğrudan deney alanı olmayacaktır; dönüşüm branch/PR üzerinden yürütülecektir.
- **REQ-032** İlk çalışma branch'i `phase-0-ko-mobile-baseline` olacaktır.
- **REQ-033** Unity proje sürümü repository içinde sabitlenecektir.
- **REQ-034** Android SDK/NDK/JDK gereksinimleri dokümante edilecektir.
- **REQ-035** Build alınması için kişisel bilgisayara özel mutlak path gerekmeyecektir.
- **REQ-036** Kaynak KO client/data path'i environment/config ile verilecektir; lisanslı olmayan dev assetler gerektiğinde repo dışında tutulabilecektir.
- **REQ-037** Converter tool sürümü generated manifest içinde yazacaktır.
- **REQ-038** Generated assetlerin source hash'i manifestte bulunacaktır.
- **REQ-039** CI, en azından Unity proje bütünlüğünü, C# compile durumunu, veri şemalarını ve converter unit testlerini doğrulayacaktır.
- **REQ-040** Android build pipeline ayrı doğrulama adımı olacaktır.
- **REQ-041** Debug APK ile release APK konfigürasyonları ayrılacaktır.
- **REQ-042** Offline test bypass sadece açık build flag/config ile etkin olacaktır.
- **REQ-043** Test bypass'ın ileride production online build'e yanlışlıkla girmesini engelleyen gate konacaktır.
- **REQ-044** Build metadata kaynak commit, project commit, data manifest hash ve converter version içerecektir.
- **REQ-045** Her parity milestone için exact commit SHA kaydedilecektir.

## 4. Unity/Android temel mimarisi

- **REQ-046** Yeni istemci Unity 6 üzerinde Android öncelikli kurulacaktır.
- **REQ-047** Yeni input katmanı Unity Input System üzerinden tasarlanacaktır; touch ve ileride keyboard/gamepad aynı action contract'ına bağlanacaktır.
- **REQ-048** Gameplay input action isimleri legacy KO komutlarıyla eşlenebilir olacaktır.
- **REQ-049** Android dokunmatik kontrolleri PC UI'sını değiştirmek yerine ayrı input overlay olarak uygulanacaktır.
- **REQ-050** Gameplay UI ve touch-control overlay mümkün olduğunca ayrıştırılacaktır.
- **REQ-051** Rendering pipeline seçimi orijinal görünümü bozmayacak en sade yol olacaktır; ilk fazda modern görsel efekt eklenmeyecektir.
- **REQ-052** Gamma/lighting/material dönüşümü kaynak görüntüyü mümkün olduğunca korumalıdır.
- **REQ-053** Kamera FOV, pitch/yaw limitleri ve karakter mesafesi referans davranıştan türetilecektir.
- **REQ-054** Unity unit scale standardı tanımlanacak; tüm KO koordinat/ölçek dönüşümü tek merkezi transform kuralından geçecektir.
- **REQ-055** DirectX handedness/axis dönüşümü tek yerde uygulanacaktır; asset başına elle düzeltme yapılmayacaktır.
- **REQ-056** Android lifecycle pause/resume sonrası offline state ve scene bozulmamalıdır.
- **REQ-057** Uygulama suspend/resume sırasında animasyon zamanlayıcıları, cooldownlar ve input state güvenli toparlanmalıdır.
- **REQ-058** Orientation ilk baseline için sabit landscape olacaktır.
- **REQ-059** Safe-area/notch alanları touch overlay'i bozmayacaktır.
- **REQ-060** Görsel parity için referans çözünürlük ve mobil ölçekleme politikası ayrı tanımlanacaktır.

## 5. Offline başlangıç ve ekran akışı

- **REQ-061** Login/ID-şifre ekranı Faz 0 APK akışından çıkarılacaktır; kaynak mantık silinmek zorunda değildir, route bypass edilecektir.
- **REQ-062** Server selection Faz 0 APK akışından çıkarılacaktır.
- **REQ-063** Nation selection görünür başlangıç ekranı olarak gösterilmeyecektir; test nation bilgisi `OfflineBootstrapProfile` ile sağlanacaktır.
- **REQ-064** Offline bootstrap hem Karus hem El Morad character-create varyantını test edebilecek yapı sunacaktır.
- **REQ-065** Varsayılan nation config ile belirlenmeli; UI içine yeni, orijinalde olmayan nation widget'ı eklenmemelidir.
- **REQ-066** Açılış route'u `Boot -> OfflineBootstrap -> CharacterCreate` olacaktır.
- **REQ-067** CharacterCreate ekranının görünür layout'u referans UI assetlerinden yeniden oluşturulacaktır.
- **REQ-068** 1.298 kaynak yapısındaki 4 race seçimi korunacaktır.
- **REQ-069** 1.298 kaynak yapısındaki 4 class seçimi korunacaktır.
- **REQ-070** STR/STA/DEX/INT/Magic Attack olmak üzere 5 başlangıç statı korunacaktır.
- **REQ-071** Başlangıç bonus point dağıtımı `NewChrValue.tbl` canonical verisinden gelmelidir.
- **REQ-072** Face left/right ve hair left/right seçim davranışı korunacaktır.
- **REQ-073** Character preview orijinal model/parts/face/hair kombinasyonunu render edecektir.
- **REQ-074** Character preview rotation davranışı mobil touch drag ile eşlenecektir; model transformu değiştirilmemelidir.
- **REQ-075** İsim alanı ve legacy isim doğrulama kuralları parity testine alınacaktır.
- **REQ-076** Offline character create sonucu network response beklemeyecek; aynı logical success/failure contract'ını local service üretecektir.
- **REQ-077** Offline kayıt benzersiz local character ID ile saklanacaktır.
- **REQ-078** Character create başarı sonrası ilk test fazında doğrudan world bootstrap yapılabilecektir.
- **REQ-079** İstenirse character-select ekranını ayrıca parity amacıyla açabilen developer route tutulacaktır; normal Faz 0 açılışında gösterilmeyecektir.
- **REQ-080** Login/server bypass kaldırıldığında CharacterCreate kodunun yeniden yazılması gerekmemelidir.

## 6. KO asset ingest ve dönüştürme pipeline'ı

- **REQ-081** OpenKO'nun N3 parserları ve OpenKO Blender eklentisi format doğrulama referansı olarak kullanılacaktır.
- **REQ-082** `.n3chr` complete character yapısı ingest edilecektir.
- **REQ-083** `.n3joint` skeleton hierarchy ingest edilecektir.
- **REQ-084** `.n3anim` animation metadata/clip ranges/events ingest edilecektir.
- **REQ-085** `.n3cpart` skinned character body/equipment parts ingest edilecektir.
- **REQ-086** `.n3cplug` weapon/equipment attachment ingest edilecektir.
- **REQ-087** `.n3pmesh` progressive/static mesh ingest edilecektir.
- **REQ-088** `.n3shape` static/environment shape ingest edilecektir.
- **REQ-089** `.dxt`/NTF DXT1/DXT3/DXT5 texture containerları decode edilecektir.
- **REQ-090** Mipmap bilgisi kaynakta varsa generated texture pipeline bunu dikkate alacaktır.
- **REQ-091** UV koordinatları parity kontrolüne alınacaktır.
- **REQ-092** Vertex position/normals/index topology dönüşümü otomatik test edilecektir.
- **REQ-093** Material slot sırası korunacaktır.
- **REQ-094** Alpha/blend/cutout davranışları kaynak material semantics'ten türetilecektir.
- **REQ-095** Attachment/socket bone ilişkileri korunacaktır.
- **REQ-096** Skinned vertex joint index ve weight değerleri normalize edilirken görsel deformasyon yaratılmamalıdır.
- **REQ-097** Dönüşümde desteklenmeyen joint count/weight durumu açık hata vermelidir.
- **REQ-098** KO tarafındaki progressive mesh/LOD verisi okunacak; Unity LOD üretiminde kaynak LOD mümkünse korunacaktır.
- **REQ-099** Kaynak mesh bounds/radius ile generated mesh bounds karşılaştırılacaktır.
- **REQ-100** Her converted character için skeleton node count ve isim/indeks mapping raporu üretilecektir.
- **REQ-101** Her converted animation için frame range, FPS, root transform ve event marker raporu üretilecektir.
- **REQ-102** Animasyonlar keyframe sayısı/clip duration parity kontrolünden geçecektir.
- **REQ-103** Converter bir dosyayı kısmen dönüştürdüğünde başarı sayılmayacaktır.
- **REQ-104** Asset batch conversion resume edilebilir olmalı; başarısız dosyalar tüm batch'i görünmez biçimde kaybettirmemelidir.
- **REQ-105** Converter çıktısı Unity importuna hazır ara format (`glTF/FBX` veya deterministic custom importer) üzerinden standardize edilecektir.
- **REQ-106** Blender yalnızca gerektiğinde conversion/validation aşaması olacaktır; runtime bağımlılığı olmayacaktır.
- **REQ-107** Mümkün olan yerde headless conversion desteklenecektir.
- **REQ-108** Generated texture, mesh ve animation dosyaları source-relative path bilgisini taşımalıdır.
- **REQ-109** Dönüşüm sonrası missing-reference taraması zorunludur.
- **REQ-110** İlk proof set: 1 tam karakter + face/hair + 1 armor set + 1 weapon + idle/walk/run/attack + 1 mob + 1 küçük map/world parçası olacaktır.

## 7. Ölçek, koordinat, mesh ve materyal parity

- **REQ-111** KO -> Blender -> Unity gibi birden fazla axis conversion üst üste uygulanmayacaktır.
- **REQ-112** Canonical coordinate transform matematiksel olarak belgelenmelidir.
- **REQ-113** Character scale `1,1,1` kaynak davranışının görsel karşılığını korumalıdır.
- **REQ-114** Weapon ve plug attachment scale character scale'den bağımsız yanlış büyütülmemelidir.
- **REQ-115** Pivot/origin farklılıkları socket offset ile telafi edilirken source transform kayıt altına alınmalıdır.
- **REQ-116** Ground contact/foot height testleri yapılacaktır.
- **REQ-117** Character capsule/collider görsel mesh'i yukarı/aşağı kaydırmayacaktır.
- **REQ-118** Static object collider generated mesh'ten ayrı optimize edilebilir; görünür mesh değişmemelidir.
- **REQ-119** Texture import ilk parity build'de mümkün olan en yüksek kaynak sadakatiyle yapılacaktır.
- **REQ-120** Android texture compression sonucu kabul edilemez görsel bozulma üretirse asset sınıfına göre istisna tanımlanacaktır.
- **REQ-121** Modern Android cihazlarda ASTC profili değerlendirilecek; geniş compatibility gerekirse ETC2 fallback ayrıca doğrulanacaktır.
- **REQ-122** Alpha channel taşıyan UI/VFX texture'ları özel kontrol edilecektir.
- **REQ-123** UI texture'larında mipmap yalnız kaynak/ölçek kullanımına uygun ise açılacaktır.
- **REQ-124** Normal map olmayan legacy texture'a otomatik normal map uydurulmayacaktır.
- **REQ-125** PBR material rework Faz 0'da yasaktır.
- **REQ-126** Specular/lighting benzeri legacy görünüm için minimal custom shader gerekirse kaynak davranışa odaklanacaktır.
- **REQ-127** Post-processing Faz 0'da eklenmeyecektir.
- **REQ-128** Color grading Faz 0'da eklenmeyecektir.
- **REQ-129** Shadow kalitesi Android performansı için değişebilir ancak model/material kimliği değiştirilemez.
- **REQ-130** Görsel optimizasyonun her türü screenshot/parity kabul testine tabi olacaktır.

## 8. Character rig ve animasyon

- **REQ-131** İlk portta KO skeleton hiyerarşisi korunacaktır.
- **REQ-132** İlk portta Generic rig tercih edilecek; Humanoid retarget rework/optimizasyon sonrası ayrı değerlendirilecektir.
- **REQ-133** Root bone mapping explicit olacaktır.
- **REQ-134** Joint orientation quaternionları dönüşümde kaybolmayacaktır.
- **REQ-135** Parent/child hierarchy sırası korunacaktır.
- **REQ-136** Skin weights kaynak joint indexleriyle eşleşecektir.
- **REQ-137** Idle/breath animasyonları korunacaktır.
- **REQ-138** Walk, run, backward hareket animasyonları korunacaktır.
- **REQ-139** Hit/struck animasyon varyantları korunacaktır.
- **REQ-140** Guard animasyonu korunacaktır.
- **REQ-141** Death varyantları kaynak ID/logic ile map edilecektir.
- **REQ-142** Sit/sit-breath/stand-up akışı korunacaktır.
- **REQ-143** Weapon-specific breath/attack animasyon aileleri korunacaktır.
- **REQ-144** Sword, dagger, dual, 2H sword, blunt, 2H blunt/axe, axe, spear, polearm, naked, bow/crossbow/launcher ve shield animation mapping kaybolmayacaktır.
- **REQ-145** Spell/magic cast A/B fazları korunacaktır.
- **REQ-146** Arrow/quarrel/javelin shoot animation mapping korunacaktır.
- **REQ-147** Skill-specific animation IDs doğrudan yeni key'lerle rastgele yeniden numaralandırılmayacaktır.
- **REQ-148** NPC breath/walk/run/attack/hit/death/talk/spell animasyon mapping'i ayrıca korunacaktır.
- **REQ-149** Animation eventleri (strike, projectile release, sound vb.) Unity AnimationEvent/custom marker sistemine taşınacaktır.
- **REQ-150** Damage timing yalnız görsel clip sonuna bağlanmayacak; legacy event/timing mantığı incelenecektir.
- **REQ-151** Attack speed animasyon playback speed ile uyumlu olacaktır.
- **REQ-152** Weapon takılı/takısız stance değişimi korunacaktır.
- **REQ-153** Face/hair/helmet görünürlük çatışmaları referans davranışa göre çözülecektir.
- **REQ-154** Animasyon geçişleri rework edilmeyecek; parity için gereksiz smoothing eklenmeyecektir.
- **REQ-155** İlk character proof'unda tüm temel animasyonlar Android üzerinde tek tek tetiklenebilir debug harness ile doğrulanacaktır.

## 9. Harita, terrain ve world ingest

- **REQ-156** `__TABLE_ZONE` alanları canonical zone tanımına dönüştürülecektir.
- **REQ-157** Terrain/GTD kaynağı ingest edilmeden heightfield uydurulmayacaktır.
- **REQ-158** Color map/TCT korunacaktır.
- **REQ-159** Light map/TLT kaynakta kullanılıyorsa parity davranışı incelenecektir.
- **REQ-160** Object post data/OPD yerleşimleri korunacaktır.
- **REQ-161** OPD extension/sub data varsa kaybedilmeyecektir.
- **REQ-162** Minimap DXT asset'i korunacaktır.
- **REQ-163** N3Sky/sky setting karşılığı Unity'de rework edilmeden taklit edilecektir.
- **REQ-164** GLO/light object bilgisi gerekiyorsa dönüştürülecektir.
- **REQ-165** GEV/event bilgisi gameplay event pipeline'a map edilecektir.
- **REQ-166** Warp/zone gate noktaları world coordinate mapping ile korunacaktır.
- **REQ-167** Collision ve walkable terrain kaynak davranışla uyuşmalıdır.
- **REQ-168** Maksimum climb/slope davranışı legacy karakter hareket mantığından doğrulanacaktır.
- **REQ-169** World object transformlarında position/rotation/scale parity testi yapılacaktır.
- **REQ-170** Mobil optimizasyon için chunk/streaming yapılabilir fakat world coordinate ve görünür yerleşim değiştirilemez.
- **REQ-171** Scene chunk sınırlarında fizik/collision boşluğu oluşmamalıdır.
- **REQ-172** Map assetleri Addressables/asset bundle benzeri sistemle remote-ready tutulabilir.
- **REQ-173** İlk offline APK gerekli proof map'i tamamen local içermelidir.
- **REQ-174** Remote asset indirme Faz 0 testinin ön koşulu olmayacaktır.
- **REQ-175** Her zone importu missing shape/texture/collision/reference raporu üretmelidir.

## 10. Orijinal HUD, skill bar ve mobil kontrol sözleşmesi

- **REQ-176** Condition/status area EXP, detailed EXP, HP, MP ve konum/minimap davranışlarını koruyacaktır.
- **REQ-177** Main function window Map, Inventory, Character, Skill, Attack, Walk/Run, Sit, Camera, Trade, Invite, Command ve Exit davranışlarını koruyan command contract'larına sahip olacaktır.
- **REQ-178** Chat modları Normal, Private, Shout, Party, Clan ve Ally için veri modeli ayrılacaktır.
- **REQ-179** Information/message output window combat ve sistem mesajlarını gösterecektir.
- **REQ-180** Target bar hedef HP/status bilgisi için ayrı component olacaktır.
- **REQ-181** Party/force UI ayrı component olarak korunacaktır.
- **REQ-182** Inventory UI ayrı component olacaktır.
- **REQ-183** Character/state UI ayrı component olacaktır.
- **REQ-184** Skill tree UI ayrı component olacaktır.
- **REQ-185** Hotkey UI ayrı component olacaktır.
- **REQ-186** Minimap ayrı component olacaktır.
- **REQ-187** Dropped item window ayrı component olacaktır.
- **REQ-188** NPC transaction/shop UI ayrı component olacaktır.
- **REQ-189** Warehouse UI ayrı component olacaktır.
- **REQ-190** Personal trade UI ayrı component olacaktır.
- **REQ-191** Quest/menu/talk UI componentleri kaynak yapıya göre korunacaktır.
- **REQ-192** Clan/knights UI componentleri sonraki parity milestone'a dahil edilecektir.
- **REQ-193** 1.298 hotbar **8 sayfa** olarak korunacaktır.
- **REQ-194** Her hotbar sayfası **8 slot** olarak korunacaktır.
- **REQ-195** PC mapping'de F1–F8 page select sözleşmesi tutulacaktır.
- **REQ-196** PC mapping'de 1–8 active slot activation sözleşmesi tutulacaktır.
- **REQ-197** Skill ve kullanılabilir item/potion hotbar'a sürüklenebilir logical entry olarak aynı slot modelini kullanacaktır.
- **REQ-198** Hotbar slot move/swap/remove davranışı korunacaktır.
- **REQ-199** Hotbar page up/down davranışı korunacaktır.
- **REQ-200** Cooldown overlay slot üzerinde gösterilecektir.
- **REQ-201** Item count tooltip/count overlay korunacaktır.
- **REQ-202** Skill tooltip davranışı mobil long-press/tap info ile erişilebilir olmalıdır.
- **REQ-203** Touch ekranında 8 skill slotun tamamı erişilebilir olacaktır.
- **REQ-204** Mobilde page switch için F-key zorunluluğu olmayacak; görünür page control/swipe mapping eklenebilir, fakat logical 8×8 yapı değişmeyecektir.
- **REQ-205** Sol analog/joystick hareketi `MoveForward/Backward/Rotate/Strafe-compatible` action contract'ına bağlanacaktır.
- **REQ-206** Sağ ekran drag kamera kontrolüne bağlanacaktır.
- **REQ-207** Target nearest enemy için mobil buton bulunacaktır; legacy Z command'ına map edilecektir.
- **REQ-208** Auto attack için mobil buton legacy R command'ına map edilecektir.
- **REQ-209** Sit/stand için mobil erişim olacaktır; legacy C command contract'ı korunacaktır.
- **REQ-210** Walk/run toggle için mobil erişim olacaktır; legacy T command contract'ı korunacaktır.
- **REQ-211** Auto-run için mobil erişim olacaktır; legacy E command contract'ı korunacaktır.
- **REQ-212** Inventory, skill, state ve map açma eylemleri touch button + keyboard fallback destekleyecektir.
- **REQ-213** Mobil overlay HUD texture'larını kalıcı rework etmemelidir; parity screenshotlarında overlay ayrı katman olarak değerlendirilecektir.
- **REQ-214** Touch target selection raycast'i eski mouse selection mantığının gameplay sonucunu korumalıdır.
- **REQ-215** Double-tap/attack shortcut gibi mobil UX ekleri core combat validation'ını bypass etmeyecektir.

## 11. Combat, skill, item ve temel gameplay parity

- **REQ-216** Target selection alive/dead/nation/NPC koşulları legacy logic'ten taşınacaktır.
- **REQ-217** Auto attack start/stop ayrı command olarak korunacaktır.
- **REQ-218** Attack range ve facing kontrolleri visual animation'dan bağımsız gameplay logic olacaktır.
- **REQ-219** Skill cast için target type, range, mana, cooldown, requirement ve state kontrolleri data driven olacaktır.
- **REQ-220** Magic/skill processing visual FX ve logical result olarak ayrılacaktır.
- **REQ-221** Skill animation ID ve effect ID mapping canonical tutulacaktır.
- **REQ-222** Skill cooldown deterministic time source kullanacaktır.
- **REQ-223** Offline testte combat clock device wall-clock manipülasyonuna ihtiyaç duymayacaktır.
- **REQ-224** HP/MP/EXP/level state modelleri ayrı fakat tek character state içinde tutarlı olacaktır.
- **REQ-225** STR/STA/DEX/INT/Magic Attack temel stat modeli korunacaktır.
- **REQ-226** Attack/Guard/resistance değerleri character stat calculation pipeline'ında korunacaktır.
- **REQ-227** Equipment slot/part/plug ilişkileri legacy ID modeline göre map edilecektir.
- **REQ-228** Item ID'leri baseline fazında değiştirilmemelidir.
- **REQ-229** Skill ID'leri baseline fazında değiştirilmemelidir.
- **REQ-230** NPC/Mob ID'leri baseline fazında değiştirilmemelidir.
- **REQ-231** Zone ID'leri baseline fazında değiştirilmemelidir.
- **REQ-232** Animation/FX ID mapping baseline fazında değiştirilmemelidir.
- **REQ-233** Inventory drag/drop logical action touch ile desteklenecektir.
- **REQ-234** Equip/unequip görünümü character mesh üzerinde aynı item mapping'i kullanacaktır.
- **REQ-235** Item durability state modellenmelidir.
- **REQ-236** Stack count state modellenmelidir.
- **REQ-237** Weight/current-max weight state modellenmelidir.
- **REQ-238** Dropped item bundle/open/get akışı offline mock world üzerinde test edilecektir.
- **REQ-239** NPC shop buy/sell contract'ı ileride server bağlanabilir interface üzerinden tasarlanacaktır.
- **REQ-240** Warehouse state ayrı storage domain olacaktır.
- **REQ-241** Party state ayrı domain model olacaktır.
- **REQ-242** Clan/knights state ayrı domain model olacaktır.
- **REQ-243** Trade state transaction-benzeri atomik domain operation olarak tasarlanacaktır.
- **REQ-244** Upgrade sistemi item mutation pipeline'ından ayrılmayacaktır; sonradan online authoritative hale getirilebilir olacaktır.
- **REQ-245** Offline proof milestone combat + drop + inventory + equip + save/reload döngüsünü tamamlamalıdır.

## 12. SQL bağımlılığının kaldırılması ve veri mimarisi

- **REQ-246** Runtime'da MSSQL kurulumu istenmeyecektir.
- **REQ-247** Runtime'da ODBC driver istenmeyecektir.
- **REQ-248** Runtime'da stored procedure çağrısı bulunmayacaktır.
- **REQ-249** Runtime'da SQL query string bulunmaması hedeflenecektir.
- **REQ-250** Legacy DB verisi yalnız migration/import kaynağı olabilir; oyun çalışırken DB engine gerektirmeyecektir.
- **REQ-251** `Item`, `Skill`, `Monster`, `NPC`, `Drop`, `Spawn`, `Zone`, `Quest`, `Upgrade`, `NewChrValue`, `PlayerLooks` gibi statik tablolar canonical data paketlerine dönüştürülecektir.
- **REQ-252** Canonical data insan tarafından inspect edilebilir source form + hızlı runtime binary form yaklaşımını destekleyebilir.
- **REQ-253** Static data paketleri schema version taşımalıdır.
- **REQ-254** Static data paketleri content hash taşımalıdır.
- **REQ-255** Static data load sırasında duplicate ID blocker sayılacaktır.
- **REQ-256** Kırık foreign reference blocker sayılacaktır.
- **REQ-257** Item->visual, skill->effect/animation, mob->visual, zone->asset referansları otomatik integrity testinden geçecektir.
- **REQ-258** Dynamic state için `ICharacterStore`, `IWarehouseStore`, `IClanStore`, `IWorldStateStore` benzeri engine-independent interface kullanılacaktır.
- **REQ-259** Faz 0 `LocalCharacterStore` uygulaması SQL'siz olacaktır.
- **REQ-260** Local state atomik/temp-file + replace veya eşdeğer crash-safe yöntemle yazılacaktır.
- **REQ-261** Yarım yazılmış save dosyası ana karakteri tamamen kaybettirmemelidir.
- **REQ-262** Save schema version/migration desteği baştan bulunmalıdır.
- **REQ-263** Character save name, nation, race, class, face, hair, stats, level, EXP, HP/MP, gold, zone, position, inventory, equipment, skills ve hotbar state'i taşımalıdır.
- **REQ-264** Hotbar 8×8 state save/reload sonrası birebir korunmalıdır.
- **REQ-265** Offline mode gerçek account password kavramı gerektirmemelidir.
- **REQ-266** Test account ID internal sabit/config olabilir; görünür login ekranı gerektirmemelidir.
- **REQ-267** Legacy `DBAgent` sorumlulukları yeni domain store/service mapping dokümanına çevrilecektir.
- **REQ-268** `CreateNewChar`, `LoadUserData`, `UpdateUser`, `Load/UpdateWarehouse`, clan operations vb. tek tek replacement contract'a map edilecektir.
- **REQ-269** SQL kaldırılırken gameplay state alanı kaybedilmemelidir.
- **REQ-270** Legacy column/record dönüşümünde alan sayısı ve ID parity raporu üretilecektir.
- **REQ-271** Import edilen statik verinin record count'u kaynakla karşılaştırılacaktır.
- **REQ-272** Numeric alanlarda overflow/truncation kontrolü yapılacaktır.
- **REQ-273** String encoding dönüşümü ayrıca test edilecektir.
- **REQ-274** Runtime static data tamamen memory/read-only registry ile servis edilebilmelidir.
- **REQ-275** SQL-free data katmanı ileride Cloudflare veya dedicated server storage adapter'ına geçişi engellememelidir.

## 13. Gelecekte online mimariye geçiş sınırı

- **REQ-276** Offline gameplay service ile online gameplay service aynı client-facing interface'i mümkün olduğunca paylaşacaktır.
- **REQ-277** Client hiçbir zaman online sürümde gold/item/EXP sonucu için nihai otorite olmayacaktır.
- **REQ-278** Network transport gameplay domain kodundan ayrılacaktır.
- **REQ-279** Legacy packet semantic isimleri mapping amacıyla korunabilir; mobil protokol gerektiğinde farklı transport kullanabilir.
- **REQ-280** Cloudflare kullanılırsa login/config/API ve uygun state coordination için ayrı adapter yazılacaktır.
- **REQ-281** Cloudflare seçimi client asset/gameplay koduna gömülmeyecektir.
- **REQ-282** Gerçek zamanlı MMO zone yükü Cloudflare kapasitesini aşarsa dedicated zone server'a geçiş mümkün olmalıdır.
- **REQ-283** WebSocket/HTTPS transport seçimi interface arkasında kalacaktır.
- **REQ-284** Online build'de offline test bypass kapalı olacaktır.
- **REQ-285** Online character create server-authoritative olacaktır.
- **REQ-286** Online item mutation server-authoritative olacaktır.
- **REQ-287** Online skill/combat validation server-authoritative olacaktır.
- **REQ-288** Online drop/loot server-authoritative olacaktır.
- **REQ-289** Online clan/trade/warehouse mutation server-authoritative olacaktır.
- **REQ-290** Network reconnect state machine ileriki faz için tasarım boşluğu bırakacaktır.

## 14. Test, parity ve kabul kapıları

- **REQ-291** "APK açıldı" tek başına başarı kabul edilmeyecektir.
- **REQ-292** Her milestone için automated acceptance matrix bulunacaktır.
- **REQ-293** Source asset SHA-256 manifest zorunludur.
- **REQ-294** Converted asset provenance manifest zorunludur.
- **REQ-295** Character mesh vertex/index/bounds kontrolü zorunludur.
- **REQ-296** Skeleton hierarchy/joint count kontrolü zorunludur.
- **REQ-297** Skin weight integrity kontrolü zorunludur.
- **REQ-298** Animation clip count/duration/event kontrolü zorunludur.
- **REQ-299** Texture dimensions/alpha/reference kontrolü zorunludur.
- **REQ-300** Item/skill/NPC/mob/zone data record count kontrolü zorunludur.
- **REQ-301** Foreign reference integrity kontrolü zorunludur.
- **REQ-302** CharacterCreate 4 race/4 class/5 stat controls test edilecektir.
- **REQ-303** Bonus point bitmeden create'in engellendiği parity test edilecektir.
- **REQ-304** Face/hair cycle test edilecektir.
- **REQ-305** Offline create/save/reload test edilecektir.
- **REQ-306** 8 hotbar page test edilecektir.
- **REQ-307** Her hotbar page'de 8 slot test edilecektir.
- **REQ-308** Hotbar drag/move/remove/save/reload test edilecektir.
- **REQ-309** Inventory equip/unequip test edilecektir.
- **REQ-310** Weapon visual attachment test edilecektir.
- **REQ-311** Idle/walk/run/backward/sit/hit/death test edilecektir.
- **REQ-312** Basic attack animation + hit timing test edilecektir.
- **REQ-313** En az bir active skill animation/VFX/cooldown test edilecektir.
- **REQ-314** Target selection ve target bar test edilecektir.
- **REQ-315** Mob damage/death/drop test edilecektir.
- **REQ-316** Loot -> inventory -> save/reload test edilecektir.
- **REQ-317** Map collision/height test edilecektir.
- **REQ-318** Camera orbit/zoom/pitch test edilecektir.
- **REQ-319** Android touch joystick test edilecektir.
- **REQ-320** Multi-touch sırasında joystick + skill aynı anda çalışabilmelidir.
- **REQ-321** Android pause/resume smoke test yapılacaktır.
- **REQ-322** Low-memory/reload davranışı test planına alınacaktır.
- **REQ-323** Missing asset durumunda fallback ile gizlemek yerine build/test fail tercih edilecektir.
- **REQ-324** Parity screenshot noktaları CharacterCreate, idle world, inventory, skill tree, hotbar, target/combat ve map için tanımlanacaktır.
- **REQ-325** Mümkün olduğunda referans PC ve Unity görüntüsü aynı kamera/karakter/item kombinasyonunda karşılaştırılacaktır.
- **REQ-326** Pixel-perfect mümkün olmayan render farkları kategori bazında açıklanacaktır; keyfi estetik fark kabul edilmeyecektir.
- **REQ-327** İlk parity milestone'da unresolved visual blocker varsa rework fazına geçilmeyecektir.
- **REQ-328** İlk parity milestone'da unresolved data integrity blocker varsa online faza geçilmeyecektir.
- **REQ-329** Her accepted milestone exact commit + manifest hash ile etiketlenecektir.
- **REQ-330** Test edilmeyen özellik "tamam" olarak işaretlenmeyecektir.

## 15. APK performans ve dağıtım şartları

- **REQ-331** İlk parity proof performanstan önce doğruluğu hedefleyecektir; ancak Android'de bellek taşması kabul edilmeyecektir.
- **REQ-332** Texture memory profili ölçülecektir.
- **REQ-333** Mesh/skin memory profili ölçülecektir.
- **REQ-334** Animation memory profili ölçülecektir.
- **REQ-335** Map/world streaming profili ölçülecektir.
- **REQ-336** Static batching/instancing yalnız görsel parity bozulmadan uygulanacaktır.
- **REQ-337** LOD sistemi kaynak LOD veya doğrulanmış türev kullanacaktır.
- **REQ-338** UI atlas optimizasyonu texture bleed/scale farkı yaratmamalıdır.
- **REQ-339** APK boyutu nedeniyle tüm world assetlerini tek pakete zorlamak yerine Addressables/Play Asset Delivery/remote-ready yapı değerlendirilecektir.
- **REQ-340** İlk offline proof için gerekli minimum asset seti APK içinde bulunacaktır.
- **REQ-341** Uygulama asset indirmeden CharacterCreate proof'una ulaşabilmelidir.
- **REQ-342** Android texture compression target cihaz sınıflarıyla doğrulanacaktır.
- **REQ-343** ARM64 release hedefi zorunlu kabul edilecektir.
- **REQ-344** Debug log spam release build'e taşınmayacaktır.
- **REQ-345** Crash logları asset/source path gibi geliştirme bilgilerini production'da gereksiz ifşa etmemelidir.

## 16. Rework sınırı

- **REQ-346** Baseline kabul edilmeden karakter adı değiştirme çalışması yapılmayacaktır.
- **REQ-347** Baseline kabul edilmeden item adı değiştirme çalışması yapılmayacaktır.
- **REQ-348** Baseline kabul edilmeden yeni armor/weapon silhouette uygulanmayacaktır.
- **REQ-349** Baseline kabul edilmeden yeni map art direction uygulanmayacaktır.
- **REQ-350** Baseline kabul edilmeden yeni skill VFX uygulanmayacaktır.
- **REQ-351** Rework başladığında gameplay ID ile visual/display ID ayrılacaktır.
- **REQ-352** Rework assetleri legacy baseline assetinin üstüne yazılmayacak; ayrı versioned content set olacaktır.
- **REQ-353** Rework sırasında aynı skeleton/animation uyumluluğu mümkünse korunacaktır.
- **REQ-354** Rework sonrası her karakter/item/skill görseli kendi yeni lisans/provenance kaydına sahip olacaktır.
- **REQ-355** Rework fazı ayrı şartname ve ayrı acceptance gate ile yürütülecektir.

---

## 17. Kaynak araştırmasından doğrulanan temel ihtiyaçlar

### 17.1 OpenKO source contract

Referans kaynak `Open-KO/KnightOnline` 1.298/1.299 sürümüne odaklanır ve resmi istemci davranışını mümkün olduğunca korumayı amaçlar. Araştırma sırasında kullanılan sabit commit:

- `https://github.com/Open-KO/KnightOnline/commit/7d6cf81093e142c928c2ac9510512b2b182178b5`
- `src/Client/WarFare/GameDef.h`
- `src/Client/WarFare/GameProcMain.h`
- `src/Client/WarFare/GameProcCharacterCreate.cpp`
- `src/Client/WarFare/UICharacterCreate.h`
- `src/Client/WarFare/UIHotKeyDlg.h/.cpp`
- `src/Client/WarFare/UISkillTreeDlg.cpp`
- `src/Client/WarFare/UIInventory.cpp`
- `src/Server/Aujard/DBAgent.h`
- `src/N3Base/N3PMesh.h`
- `src/N3Base/N3Skin.h`
- `src/N3Base/N3Joint.h`

### 17.2 Asset format referansı

OpenKO'nun Blender eklentisi Knight Online varlıkları için doğrudan kullanılabilir parser/validation referansıdır:

- `https://github.com/Open-KO/OpenKO-blender`
- `.n3chr`: complete character
- `.n3shape`: static/environment shape
- `.n3cpart`: skinned character part
- `.n3cplug`: equipment/weapon attachment
- `.n3joint`: skeleton hierarchy
- `.n3anim`: animation metadata/events
- `.n3pmesh`: progressive mesh/LOD
- `.dxt/.ntf`: DXT texture container

Dosya formatı açıklamaları:

- `https://github.com/Open-KO/OpenKO-blender/blob/main/docs/02-file-format-specs.md`

### 17.3 1.298 UI/hotbar doğrulaması

Source `GameDef.h` F1–F8 skill page ve 1–8 hotkey mapping'ini içerir. `UIHotKeyDlg` çok sayfalı hotbar state, drag/drop, cooldown ve tooltip/count davranışını taşır. Dönemin kullanıcı rehberleri de 8 hotbar sayfası × 8 slot yapısını tarif eder:

- `https://www.harbiforum.net/konu/knight-online-yeni-baslayanlar.81948/`
- `https://forum.paticik.com/topic/1453906-oyun-hakkynda-bilgiler-ssss-g03112005/`

### 17.4 Unity/Android referansları

- Input System: `https://docs.unity3d.com/6000.0/Documentation/Manual/Input.html`
- Generic/Humanoid rig import: `https://docs.unity3d.com/6000.0/Documentation/Manual/FBXImporter-Rig.html`
- Runtime asset management/Addressables: `https://docs.unity3d.com/6000.0/Documentation/Manual/assets-managing-introduction.html`
- Addressables for Android: `https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.addressables.android.html`
- Android texture formats: `https://docs.unity3d.com/6000.0/Documentation/Manual/texture-choose-format-by-platform.html`

---

## 18. Faz 0 çıkış kriteri

Faz 0 ancak aşağıdaki zincir **SQL/server olmadan Android APK üzerinde** tamamlanırsa başarı sayılacaktır:

`APK boot -> CharacterCreate -> race/class/stat/face/hair/name -> local create -> world load -> original character render -> movement/camera -> inventory -> skill tree -> 8x8 hotbar -> target -> basic attack -> skill -> mob death -> drop -> loot -> equipment -> local save -> APK restart -> state restore`

Bu zincirde herhangi bir adım mock ekran görüntüsüyle veya çalışmayan placeholder button ile geçilmiş sayılmayacaktır. Her adım gerçek state mutation üretmelidir.
