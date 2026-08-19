# MMORPG Mobile Port

Bu depo, Knight Online 1.298/1.299 davranışını ve görsel varlık düzenini ilk ayağa kaldırma aşamasında mümkün olduğunca değiştirmeden Android/Unity tabanlı bir APK istemcisine taşımak için oluşturulmuştur.

## Faz 0 hedefi

- Kaynak referans: `Open-KO/KnightOnline` commit `7d6cf81093e142c928c2ac9510512b2b182178b5`.
- İlk mobil prototipte görünür rework yapılmayacak.
- Login/ID-şifre ekranı ve server selection ilk offline test yapısında atlanacak.
- APK doğrudan karakter oluşturma prosedürüyle başlayacak.
- SQL/MSSQL/ODBC çalışma zamanı bağımlılığı yeni projede zorunlu olmayacak.
- İlk doğrulama hedefi: karakter oluşturma -> world bootstrap -> hareket -> kamera -> inventory -> skill tree -> 8x8 hotbar -> target -> attack/skill -> item/drop döngüsü.
- Daha sonraki rework fazı, çalışan baseline doğrulandıktan sonra ayrı yürütülecek.

Bağlayıcı teknik kurallar için `docs/00-SARTNAME.md`, uygulama sırası için `docs/01-YAPILACAKLAR.md` kullanılacaktır.
