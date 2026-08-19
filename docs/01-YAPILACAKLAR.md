# MMORPG — KO 1.298 Mobil Dönüşüm Yapılacaklar Listesi

Bu liste `docs/00-SARTNAME.md` içindeki bağlayıcı gereksinimlerden türetilmiştir. Bir madde yalnız kod yazıldığı için değil, ilgili acceptance testleri geçtiğinde tamamlanmış sayılır.

## Durum tanımları

- `[ ]` yapılmadı / doğrulanmadı
- `[x]` tamamlandı ve doğrulandı
- Bir fazın gate'i geçmeden sonraki fazdaki rework/özgünleştirme işlerine başlanmaz.

---

# P0 — Kaynağı dondur, proje temelini kur

- [ ] **P0.001** OpenKO referans commit `7d6cf81093e142c928c2ac9510512b2b182178b5` için source manifest oluştur. (`REQ-005–006`)
- [ ] **P0.002** Referans KO client/data paketinin exact dosya listesini çıkar. (`REQ-007–009`)
- [ ] **P0.003** Tüm source assetler için SHA-256 manifest üret. (`REQ-008`, `REQ-293`)
- [ ] **P0.004** Asset provenance şemasını tanımla: source path, source hash, generated path, converter version. (`REQ-021–030`)
- [ ] **P0.005** `docs/research/` klasörünü oluştur ve araştırma kaynaklarını kaydet. (`REQ-030`)
- [ ] **P0.006** Unity 6 proje sürümünü seç ve `ProjectVersion.txt` ile sabitle. (`REQ-033`, `REQ-046`)
- [ ] **P0.007** Android SDK/NDK/JDK requirement dokümanını ekle. (`REQ-034`)
- [ ] **P0.008** Unity klasör yapısını oluştur: `Assets/Game`, `Assets/Generated`, `Assets/Editor`, `Assets/Tests`, `Packages`, `Tools`. (`REQ-035–040`)
- [ ] **P0.009** Source KO data path'ini environment/config üzerinden al; absolute path hardcode etme. (`REQ-035–036`)
- [ ] **P0.010** Debug/release/offline build flags şemasını kur. (`REQ-041–043`)
- [ ] **P0.011** Build metadata modelini oluştur: project SHA, OpenKO SHA, data manifest hash, converter version. (`REQ-044–045`)
- [ ] **P0.012** İlk CI workflow iskeletini oluştur: C# compile + EditMode tests + data validation. (`REQ-039–040`)

**P0 Gate:** Kaynak sürüm ve assetler hash ile sabitlenmeden converter yazımına geçilmez.

---

# P1 — N3 asset conversion proof

- [ ] **P1.001** OpenKO `N3PMesh`, `N3Skin`, `N3Joint`, `N3Chr` kodlarını format referansı olarak sabitle. (`REQ-081`)
- [ ] **P1.002** OpenKO Blender parser testlerini incele ve desteklenen format matrisini repo dokümanına geçir. (`REQ-081–089`)
- [ ] **P1.003** `.dxt/.ntf` DXT1 decoder proof oluştur. (`REQ-089–090`)
- [ ] **P1.004** DXT3 alpha decoder proof oluştur. (`REQ-089`, `REQ-122`)
- [ ] **P1.005** DXT5 alpha decoder proof oluştur. (`REQ-089`, `REQ-122`)
- [ ] **P1.006** `.n3pmesh` vertex/index/UV/material parser proof oluştur. (`REQ-087`, `REQ-091–099`)
- [ ] **P1.007** `.n3joint` hierarchy/orientation parser proof oluştur. (`REQ-083`, `REQ-100`, `REQ-131–136`)
- [ ] **P1.008** `.n3skin` joint indices + weights parser proof oluştur. (`REQ-096–097`, `REQ-136`)
- [ ] **P1.009** `.n3anim` clip/event metadata parser proof oluştur. (`REQ-084`, `REQ-101–102`, `REQ-149`)
- [ ] **P1.010** `.n3cpart` body/equipment part parser proof oluştur. (`REQ-085`)
- [ ] **P1.011** `.n3cplug` weapon attachment parser proof oluştur. (`REQ-086`, `REQ-095`)
- [ ] **P1.012** `.n3chr` aggregate character loader proof oluştur. (`REQ-082`)
- [ ] **P1.013** `.n3shape` static object parser proof oluştur. (`REQ-088`)
- [ ] **P1.014** DirectX -> canonical coordinate transform testlerini yaz. (`REQ-054–055`, `REQ-111–112`)
- [ ] **P1.015** Canonical -> Unity coordinate transform testlerini yaz. (`REQ-111–117`)
- [ ] **P1.016** Aynı conversion'ın iki çalıştırmada aynı logical manifesti ürettiğini doğrula. (`REQ-010`, `REQ-037–038`)
- [ ] **P1.017** Unsupported/unknown field raporlama mekanizması ekle. (`REQ-011–012`, `REQ-103–104`)
- [ ] **P1.018** İlk tam karakteri mesh+skeleton+skin+texture olarak Unity'ye al. (`REQ-110`)
- [ ] **P1.019** İlk weapon plug'ını doğru bone/socket'a bağla. (`REQ-095`, `REQ-114–115`)
- [ ] **P1.020** İlk idle/walk/run/attack clip setini Unity'ye aktar. (`REQ-102`, `REQ-137–151`)
- [ ] **P1.021** Character proof için vertex/index/bounds/joint/weight/animation manifest raporu üret. (`REQ-295–299`)

**P1 Gate:** Bir tam KO karakteri Unity Scene'de doğru texture, doğru skeleton ve en az idle/walk/run/attack ile bozulmadan görünmelidir.

---

# P2 — Unity Android render ve input baseline

- [ ] **P2.001** Landscape Android player settings'i kur. (`REQ-058`)
- [ ] **P2.002** ARM64 hedefini etkinleştir. (`REQ-343`)
- [ ] **P2.003** Unity Input System paketini kur ve action map oluştur. (`REQ-047–050`)
- [ ] **P2.004** `Move`, `CameraLook`, `TargetNearest`, `AutoAttack`, `Sit`, `WalkRun`, `AutoRun`, `Inventory`, `Skill`, `State`, `Map`, `Hotbar1..8`, `HotbarPage1..8` action'larını tanımla. (`REQ-048`, `REQ-195–215`)
- [ ] **P2.005** Touch joystick proof oluştur. (`REQ-205`)
- [ ] **P2.006** Sağ ekran camera drag proof oluştur. (`REQ-206`)
- [ ] **P2.007** Multi-touch joystick + skill press testi yaz. (`REQ-320`)
- [ ] **P2.008** Safe-area/notch layout sistemini kur. (`REQ-059`)
- [ ] **P2.009** Legacy-look minimal shader/material proof oluştur. (`REQ-051–052`, `REQ-125–130`)
- [ ] **P2.010** ASTC/ETC2 texture import profillerini tanımla; kaynak texture ile görsel karşılaştırma yap. (`REQ-119–123`, `REQ-342`)
- [ ] **P2.011** Android pause/resume state hooklarını ekle. (`REQ-056–057`, `REQ-321`)
- [ ] **P2.012** İlk debug APK'yı CI/local build üzerinden üret. (`REQ-040–045`)

**P2 Gate:** İlk karakter Android APK'da render edilmeli; joystick ile hareket action'ı ve camera drag aynı anda çalışmalıdır.

---

# P3 — Offline bootstrap ve orijinal CharacterCreate

- [ ] **P3.001** `OfflineBootstrapProfile` modelini oluştur. (`REQ-063–066`)
- [ ] **P3.002** Profile'a `nation`, internal test account ID, character slot ve initial zone alanlarını ekle. (`REQ-064–066`, `REQ-265–266`)
- [ ] **P3.003** `Boot -> OfflineBootstrap -> CharacterCreate` route'unu kur. (`REQ-061–066`)
- [ ] **P3.004** Login procedure'ünü silmeden offline route'tan bypass et. (`REQ-061`, `REQ-080`)
- [ ] **P3.005** Server selection procedure'ünü silmeden bypass et. (`REQ-062`, `REQ-080`)
- [ ] **P3.006** Nation selection'ı görünür ekransız bootstrap config'e bağla. (`REQ-063–065`)
- [ ] **P3.007** Developer config ile Karus ve El Morad CharacterCreate varyantlarını ayrı açabil. (`REQ-064`)
- [ ] **P3.008** Original CharacterCreate UI resource layout'ını Unity UI'ye çıkar. (`REQ-067`)
- [ ] **P3.009** 4 race selector'ı bağla. (`REQ-068`, `REQ-302`)
- [ ] **P3.010** 4 class selector'ı bağla. (`REQ-069`, `REQ-302`)
- [ ] **P3.011** 5 stat alanını bağla. (`REQ-070`, `REQ-302`)
- [ ] **P3.012** `NewChrValue.tbl` importer'ını yaz; başlangıç stat/bonus değerlerini buradan getir. (`REQ-071`, `REQ-251`)
- [ ] **P3.013** Race/class kombinasyonuna göre disabled class davranışını taşımaya başla. (`REQ-067–071`)
- [ ] **P3.014** Face left/right davranışını bağla. (`REQ-072`, `REQ-304`)
- [ ] **P3.015** Hair left/right davranışını bağla. (`REQ-072`, `REQ-304`)
- [ ] **P3.016** Character preview parts/face/hair renderer'ını bağla. (`REQ-073`)
- [ ] **P3.017** Touch drag character rotate ekle. (`REQ-074`)
- [ ] **P3.018** Legacy name validation kurallarını port et. (`REQ-075`)
- [ ] **P3.019** Bonus point > 0 ise create'i engelle. (`REQ-303`)
- [ ] **P3.020** Offline `CreateCharacterService` success/failure contract'ını yaz. (`REQ-076`)
- [ ] **P3.021** Developer-only CharacterSelect route'u hazır tut. (`REQ-079`)
- [ ] **P3.022** CharacterCreate screenshot reference/golden noktalarını tanımla. (`REQ-324–326`)

**P3 Gate:** APK temiz kurulumda hiçbir ID/şifre/server ekranı göstermeden orijinal görünümlü CharacterCreate ekranına ulaşmalı ve gerçek character state oluşturmalıdır.

---

# P4 — SQL'siz canonical data katmanı

- [ ] **P4.001** Canonical data schema versioning standardını tanımla. (`REQ-251–254`)
- [ ] **P4.002** Item table importer yaz. (`REQ-251`)
- [ ] **P4.003** Skill/magic table importer yaz. (`REQ-251`)
- [ ] **P4.004** Monster/NPC importer yaz. (`REQ-251`)
- [ ] **P4.005** Drop table importer yaz. (`REQ-251`)
- [ ] **P4.006** Spawn/event importer yaz. (`REQ-251`)
- [ ] **P4.007** Zone table importer yaz. (`REQ-156`, `REQ-251`)
- [ ] **P4.008** Quest data importer yaz. (`REQ-251`)
- [ ] **P4.009** Upgrade data importer yaz. (`REQ-251`)
- [ ] **P4.010** PlayerLooks importer yaz. (`REQ-251`)
- [ ] **P4.011** Duplicate-ID validation ekle. (`REQ-255`)
- [ ] **P4.012** Foreign-reference validation ekle. (`REQ-256–257`)
- [ ] **P4.013** Source record count vs canonical record count testleri ekle. (`REQ-270–271`, `REQ-300`)
- [ ] **P4.014** Numeric overflow/truncation testleri ekle. (`REQ-272`)
- [ ] **P4.015** Legacy string encoding testleri ekle. (`REQ-273`)
- [ ] **P4.016** Runtime read-only registry oluştur. (`REQ-274`)
- [ ] **P4.017** Kod tabanında runtime SQL/MSSQL/ODBC dependency scanner gate oluştur. (`REQ-246–250`)
- [ ] **P4.018** Generated data package content hash üret. (`REQ-253–254`)

**P4 Gate:** Character/item/skill/mob/zone proof'u DB motoru kurmadan canonical paketlerden yüklenmelidir.

---

# P5 — SQL'siz dynamic save/persistence

- [ ] **P5.001** `ICharacterStore` interface oluştur. (`REQ-258`)
- [ ] **P5.002** `IWarehouseStore` interface oluştur. (`REQ-258`)
- [ ] **P5.003** `IClanStore` interface oluştur. (`REQ-258`)
- [ ] **P5.004** `IWorldStateStore` interface oluştur. (`REQ-258`)
- [ ] **P5.005** `LocalCharacterStore` SQL'siz implementation yaz. (`REQ-259`)
- [ ] **P5.006** Crash-safe temp+atomic replace save yöntemini uygula. (`REQ-260–261`)
- [ ] **P5.007** Save schema version + migration framework oluştur. (`REQ-262`)
- [ ] **P5.008** Character identity/nation/race/class/face/hair state'i kaydet. (`REQ-263`)
- [ ] **P5.009** Stats/level/EXP/HP/MP/gold state'i kaydet. (`REQ-263`)
- [ ] **P5.010** Zone/position state'i kaydet. (`REQ-263`)
- [ ] **P5.011** Inventory/equipment state'i kaydet. (`REQ-263`)
- [ ] **P5.012** Skills state'i kaydet. (`REQ-263`)
- [ ] **P5.013** 8×8 hotbar state'i kaydet. (`REQ-264`)
- [ ] **P5.014** Save corruption/recovery testleri yaz. (`REQ-260–262`)
- [ ] **P5.015** `DBAgent` API -> yeni domain store/service mapping dokümanını oluştur. (`REQ-267–269`)
- [ ] **P5.016** `CreateNewChar` replacement contract'ını kapat. (`REQ-268`)
- [ ] **P5.017** `LoadUserData`/`UpdateUser` replacement contract'ını kapat. (`REQ-268`)
- [ ] **P5.018** Warehouse replacement contract'ını tanımla. (`REQ-268`)
- [ ] **P5.019** Clan replacement contract'ını tanımla. (`REQ-268`)

**P5 Gate:** APK kapatılıp açıldığında oluşturulan karakter ve mevcut proof state DB olmadan geri gelmelidir.

---

# P6 — World/map port proof

- [ ] **P6.001** `__TABLE_ZONE` -> `ZoneDefinition` mapping oluştur. (`REQ-156`)
- [ ] **P6.002** Terrain/GTD importer proof oluştur. (`REQ-157`)
- [ ] **P6.003** Color map/TCT bağla. (`REQ-158`)
- [ ] **P6.004** Light map/TLT kullanımını kaynak koddan doğrula ve gerekiyorsa bağla. (`REQ-159`)
- [ ] **P6.005** OPD object placement importer yaz. (`REQ-160–161`)
- [ ] **P6.006** Minimap DXT importunu bağla. (`REQ-162`)
- [ ] **P6.007** N3Sky eşdeğer sky setup proof oluştur. (`REQ-163`)
- [ ] **P6.008** GLO/GEV/event references'i kaybetmeden canonical metadata'ya taşı. (`REQ-164–165`)
- [ ] **P6.009** Warp/gate coordinate mapping oluştur. (`REQ-166`)
- [ ] **P6.010** Terrain height/collision doğrulamasını yaz. (`REQ-167–168`, `REQ-317`)
- [ ] **P6.011** Static object position/rotation/scale parity testlerini yaz. (`REQ-169`)
- [ ] **P6.012** İlk küçük world proof sahnesini Android'de çalıştır. (`REQ-173`)
- [ ] **P6.013** Missing map reference report üret. (`REQ-175`)
- [ ] **P6.014** Gerekirse chunk/streaming prototipi yap; coordinate parity'yi test et. (`REQ-170–172`)

**P6 Gate:** Proof map'te karakter zemine doğru oturmalı, collision/height çalışmalı ve kaynak object yerleşimleri kaymamalıdır.

---

# P7 — Original HUD ve 8×8 hotbar

- [ ] **P7.001** Status/condition UI: EXP, detailed EXP, HP, MP, location/minimap. (`REQ-176`)
- [ ] **P7.002** Function window command modelini kur. (`REQ-177`)
- [ ] **P7.003** Message/information output window'u kur. (`REQ-179`)
- [ ] **P7.004** Target bar componentini kur. (`REQ-180`)
- [ ] **P7.005** Inventory componentini kur. (`REQ-182`)
- [ ] **P7.006** Character/state componentini kur. (`REQ-183`)
- [ ] **P7.007** Skill tree componentini kur. (`REQ-184`)
- [ ] **P7.008** Hotbar componentini kur. (`REQ-185`)
- [ ] **P7.009** Minimap componentini kur. (`REQ-186`)
- [ ] **P7.010** Hotbar modelini exact 8 page yap. (`REQ-193`, `REQ-306`)
- [ ] **P7.011** Her page'i exact 8 slot yap. (`REQ-194`, `REQ-307`)
- [ ] **P7.012** Skill -> hotbar drag/drop ekle. (`REQ-197–198`)
- [ ] **P7.013** Usable item/potion -> hotbar drag/drop ekle. (`REQ-197`)
- [ ] **P7.014** Slot move/swap/remove ekle. (`REQ-198`, `REQ-308`)
- [ ] **P7.015** Page up/down + page select ekle. (`REQ-199`, `REQ-204`)
- [ ] **P7.016** Cooldown overlay ekle. (`REQ-200`)
- [ ] **P7.017** Item count overlay ekle. (`REQ-201`)
- [ ] **P7.018** Tooltip/long-press info ekle. (`REQ-202`)
- [ ] **P7.019** Hotbar state'i LocalCharacterStore'a bağla. (`REQ-264`)
- [ ] **P7.020** Mobil page control/swipe ekle; logical 8×8'i değiştirme. (`REQ-203–204`)
- [ ] **P7.021** Inventory/Skill/State/Map touch shortcuts ekle. (`REQ-212`)
- [ ] **P7.022** HUD overlay ile parity UI katmanlarını ayrıştır. (`REQ-213`)
- [ ] **P7.023** 8×8 hotbar full automated persistence testini yaz. (`REQ-306–308`)

**P7 Gate:** Skill ve potionlar 8 ayrı sayfanın 8 slotuna taşınabilmeli, kullanılabilmeli ve APK restart sonrası aynı yerde kalmalıdır.

---

# P8 — Hareket, kamera, target ve combat proof

- [ ] **P8.001** Legacy walk/run hız ve state modelini kaynak koddan port et. (`REQ-137–144`, `REQ-216–218`)
- [ ] **P8.002** Forward/backward hareketi bağla. (`REQ-138`)
- [ ] **P8.003** Turn/rotation davranışını bağla. (`REQ-205–206`)
- [ ] **P8.004** Sit/stand state + animations bağla. (`REQ-142`, `REQ-209`)
- [ ] **P8.005** Walk/run toggle bağla. (`REQ-210`)
- [ ] **P8.006** Auto-run bağla. (`REQ-211`)
- [ ] **P8.007** Camera orbit/pitch/zoom parity davranışını kur. (`REQ-053`, `REQ-318`)
- [ ] **P8.008** Touch target raycast/selection bağla. (`REQ-214`, `REQ-216`)
- [ ] **P8.009** Nearest enemy command'ını mobil target butonuna bağla. (`REQ-207`)
- [ ] **P8.010** Target bar HP/state güncellemesini bağla. (`REQ-180`, `REQ-314`)
- [ ] **P8.011** Auto attack start/stop'u bağla. (`REQ-208`, `REQ-217`)
- [ ] **P8.012** Attack range/facing validation yaz. (`REQ-218`)
- [ ] **P8.013** Basic weapon attack animation mapping'i bağla. (`REQ-143–144`, `REQ-312`)
- [ ] **P8.014** Attack hit timing marker'ını bağla. (`REQ-149–151`)
- [ ] **P8.015** Hit/struck animasyonlarını bağla. (`REQ-139`)
- [ ] **P8.016** Death animasyonlarını bağla. (`REQ-141`)

**P8 Gate:** Android'de target -> auto attack -> hit -> HP düşüşü -> death zinciri gerçek state mutation ile çalışmalıdır.

---

# P9 — Skill, VFX ve projectile proof

- [ ] **P9.001** Skill definition runtime modelini oluştur. (`REQ-219–222`)
- [ ] **P9.002** Target type validation ekle. (`REQ-219`)
- [ ] **P9.003** Range validation ekle. (`REQ-219`)
- [ ] **P9.004** MP/mana cost validation ekle. (`REQ-219`)
- [ ] **P9.005** Cooldown state/time source ekle. (`REQ-219–223`)
- [ ] **P9.006** Required level/class/stat requirement mapping'i ekle. (`REQ-219`)
- [ ] **P9.007** Skill animation ID -> Unity clip mapping'i kur. (`REQ-145–149`, `REQ-221`)
- [ ] **P9.008** Skill effect ID -> Unity visual prefab mapping'i kur. (`REQ-220–221`)
- [ ] **P9.009** Cast event marker'larını bağla. (`REQ-149–150`)
- [ ] **P9.010** Projectile release marker'ını bağla. (`REQ-146`, `REQ-149`)
- [ ] **P9.011** Impact VFX timing'ini bağla. (`REQ-220`)
- [ ] **P9.012** En az bir melee active skill'i baştan sona port et. (`REQ-313`)
- [ ] **P9.013** En az bir ranged/projectile skill'i baştan sona port et. (`REQ-146`, `REQ-313`)
- [ ] **P9.014** En az bir magic cast skill'i baştan sona port et. (`REQ-145`, `REQ-313`)
- [ ] **P9.015** Hotbar cooldown ve skill state'i bağla. (`REQ-200`, `REQ-222`)

**P9 Gate:** Hotbar'dan kullanılan en az üç farklı skill ailesi animasyon + VFX + gerçek MP/cooldown/damage state ile çalışmalıdır.

---

# P10 — Item, inventory, drop ve equipment proof

- [ ] **P10.001** Baseline item ID mapping'ini sabitle. (`REQ-228`)
- [ ] **P10.002** Inventory slot modelini kur. (`REQ-233`)
- [ ] **P10.003** Equipment slot/part/plug modelini kur. (`REQ-227`)
- [ ] **P10.004** Equip/unequip state mutation yaz. (`REQ-234`, `REQ-309`)
- [ ] **P10.005** Equipped weapon/armor visual mapping'i character'a bağla. (`REQ-234`, `REQ-310`)
- [ ] **P10.006** Durability modelini ekle. (`REQ-235`)
- [ ] **P10.007** Stack count modelini ekle. (`REQ-236`)
- [ ] **P10.008** Weight/max weight modelini ekle. (`REQ-237`)
- [ ] **P10.009** Offline mob drop definition/roll proof oluştur. (`REQ-238`)
- [ ] **P10.010** Dropped bundle world representation'ı oluştur. (`REQ-187`, `REQ-238`)
- [ ] **P10.011** Touch loot/open/get akışını bağla. (`REQ-238`)
- [ ] **P10.012** Loot edilen itemi inventory'ye yaz. (`REQ-238`)
- [ ] **P10.013** Inventory + equipment'i local save/reload'a bağla. (`REQ-263`, `REQ-316`)
- [ ] **P10.014** Mob death -> drop -> loot -> equip -> restart -> restore E2E testini yaz. (`REQ-245`, `REQ-315–316`)

**P10 Gate:** Bir mob kesilip gerçek item düşmeli, item alınmalı, takılmalı ve APK restart sonrası takılı kalmalıdır.

---

# P11 — Kalan 1.298 core UI/domain ihtiyaçları

- [ ] **P11.001** Party/force UI + state modelini ekle. (`REQ-181`, `REQ-241`)
- [ ] **P11.002** Chat domain modelini Normal/Private/Shout/Party/Clan/Ally olarak kur. (`REQ-178`)
- [ ] **P11.003** Dropped item UI'yi parity layout'a yaklaştır. (`REQ-187`)
- [ ] **P11.004** NPC transaction/shop UI contract'ını ekle. (`REQ-188`, `REQ-239`)
- [ ] **P11.005** Warehouse UI + state contract'ını ekle. (`REQ-189`, `REQ-240`)
- [ ] **P11.006** Personal trade UI + atomic trade domain tasarımını ekle. (`REQ-190`, `REQ-243`)
- [ ] **P11.007** Quest menu/talk/content modellerini ekle. (`REQ-191`)
- [ ] **P11.008** Clan/knights state + UI domain modelini ekle. (`REQ-192`, `REQ-242`)
- [ ] **P11.009** Upgrade/anvil domain modelini canonical data ile bağla. (`REQ-244`)
- [ ] **P11.010** Warp/zone-change UI ve state transition'ı bağla. (`REQ-166`)
- [ ] **P11.011** Exit/return flow'u Android lifecycle'a uyumlu hale getir. (`REQ-056–057`)

**P11 Gate:** Ana ekran yalnız karakter+combat prototipi değil, 1.298 core UI/domain mimarisinin eksik pencerelerini de taşıyan genişletilebilir yapı olmalıdır.

---

# P12 — Parity audit ve APK kabul turu

- [ ] **P12.001** CharacterCreate golden screenshot oluştur. (`REQ-324`)
- [ ] **P12.002** Idle world golden screenshot oluştur. (`REQ-324`)
- [ ] **P12.003** Inventory golden screenshot oluştur. (`REQ-324`)
- [ ] **P12.004** Skill tree golden screenshot oluştur. (`REQ-324`)
- [ ] **P12.005** Hotbar golden screenshot oluştur. (`REQ-324`)
- [ ] **P12.006** Target/combat golden screenshot oluştur. (`REQ-324`)
- [ ] **P12.007** Map/minimap golden screenshot oluştur. (`REQ-324`)
- [ ] **P12.008** Source vs generated asset manifest audit yap. (`REQ-293–301`)
- [ ] **P12.009** CharacterCreate race/class/stat/bonus/face/hair tests çalıştır. (`REQ-302–305`)
- [ ] **P12.010** Hotbar 8×8 full tests çalıştır. (`REQ-306–308`)
- [ ] **P12.011** Inventory/equipment tests çalıştır. (`REQ-309–310`)
- [ ] **P12.012** Animation suite çalıştır. (`REQ-311–313`)
- [ ] **P12.013** Target/combat/mob/drop tests çalıştır. (`REQ-314–316`)
- [ ] **P12.014** Map collision/height tests çalıştır. (`REQ-317`)
- [ ] **P12.015** Camera tests çalıştır. (`REQ-318`)
- [ ] **P12.016** Touch/multi-touch tests çalıştır. (`REQ-319–320`)
- [ ] **P12.017** Pause/resume tests çalıştır. (`REQ-321`)
- [ ] **P12.018** Missing asset ve broken reference gate'ini çalıştır. (`REQ-323`)
- [ ] **P12.019** SQL/MSSQL/ODBC runtime dependency scan'i çalıştır. (`REQ-246–250`)
- [ ] **P12.020** Offline tam E2E zincirini temiz kurulumda çalıştır. (`REQ-330`, Faz 0 çıkış kriteri)
- [ ] **P12.021** Exact commit + manifest hash'i milestone olarak kaydet. (`REQ-329`)

**P12 Gate — Faz 0 Final:**  
`APK -> CharacterCreate -> world -> move/camera -> inventory -> skill tree -> 8x8 hotbar -> target -> attack/skill -> mob -> drop -> loot -> equip -> save -> restart -> restore` zinciri gerçek şekilde geçmeden Faz 0 tamamlanmış değildir.

---

# P13 — Performans/asset delivery (parity sonrasında, rework öncesinde)

- [ ] **P13.001** Texture memory profile çıkar. (`REQ-332`)
- [ ] **P13.002** Mesh/skin memory profile çıkar. (`REQ-333`)
- [ ] **P13.003** Animation memory profile çıkar. (`REQ-334`)
- [ ] **P13.004** World streaming memory profile çıkar. (`REQ-335`)
- [ ] **P13.005** Legacy/source LOD mapping'ini doğrula. (`REQ-337`)
- [ ] **P13.006** UI atlas optimizasyonunu parity screenshot ile doğrula. (`REQ-338`)
- [ ] **P13.007** Addressables gruplarını character/item/map/UI olarak ayır. (`REQ-339–341`)
- [ ] **P13.008** Play Asset Delivery / remote-ready strategy tasarla. (`REQ-339`)
- [ ] **P13.009** Offline proof asset setinin internetsiz açıldığını tekrar doğrula. (`REQ-340–341`)
- [ ] **P13.010** ASTC/ETC2 cihaz matrisi test planını uygula. (`REQ-342`)

---

# P14 — Online server adapter (Faz 0'dan sonra)

- [ ] **P14.001** Offline service interface'lerini online implementation için dondur. (`REQ-276`)
- [ ] **P14.002** Transport abstraction oluştur. (`REQ-278–283`)
- [ ] **P14.003** HTTPS/WebSocket client adapter proof oluştur. (`REQ-283`)
- [ ] **P14.004** Online build'de offline bypass'ı CI ile yasakla. (`REQ-284`)
- [ ] **P14.005** Character create authority server'a taşı. (`REQ-285`)
- [ ] **P14.006** Item mutation authority server'a taşı. (`REQ-286`)
- [ ] **P14.007** Combat/skill authority server'a taşı. (`REQ-287`)
- [ ] **P14.008** Drop/loot authority server'a taşı. (`REQ-288`)
- [ ] **P14.009** Clan/trade/warehouse authority server'a taşı. (`REQ-289`)
- [ ] **P14.010** Cloudflare adapter ile dedicated zone server adapter'ını client'tan bağımsız tut. (`REQ-280–282`)

---

# P15 — Rework başlangıcı — BU FAZ ŞİMDİLİK KİLİTLİ

Aşağıdaki maddeler yalnız P12 Faz 0 Final gate geçtikten sonra açılacaktır.

- [ ] **P15.001** Gameplay ID / Visual ID / Display ID ayrımını yap. (`REQ-351`)
- [ ] **P15.002** Legacy content seti ile rework content setini ayrı versionla. (`REQ-352`)
- [ ] **P15.003** Yeni oyun adı/marka değişikliklerini uygula. (`REQ-346–355`)
- [ ] **P15.004** Karakter concept/rework pipeline'ını başlat. (`REQ-348`, `REQ-353–354`)
- [ ] **P15.005** Item/weapon/armor rework pipeline'ını başlat. (`REQ-347–348`)
- [ ] **P15.006** Yeni skill VFX setini başlat. (`REQ-350`)
- [ ] **P15.007** Harita sanat yönetimi rework'ünü başlat. (`REQ-349`)
- [ ] **P15.008** Rework için ayrı şartname ve acceptance gate oluştur. (`REQ-355`)

---

## İlk uygulanacak dar çalışma paketi

Kodlamaya başlanacak ilk gerçek dikey dilim şudur:

1. Kaynak asset manifesti.
2. Unity 6 Android proje iskeleti.
3. OpenKO/OpenKO-Blender referansıyla **tek tam karakter** importu.
4. Skeleton + skin weights + texture.
5. Idle + walk + run + basic attack.
6. Tek weapon plug.
7. Basit touch joystick + camera drag.
8. Offline bootstrap.
9. Original CharacterCreate UI.
10. Race/class/stat/face/hair/name ile local character state yaratma.
11. Tek küçük world/map proof.
12. Android debug APK.

Bu 12 madde başarıyla doğrulanmadan bulk item/map/skill dönüşümüne geçilmemelidir; aksi halde converter veya coordinate/rig hatası binlerce assete çoğaltılmış olur.
