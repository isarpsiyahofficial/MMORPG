# Legacy KO Source (local only)

Bu klasör yalnız geliştirme bilgisayarında kullanılacak orijinal KO test dosyaları içindir.

İlk hedef için buraya tek bir tam karakter seti konur:

- `.n3chr` karakter dosyası
- karakterin kullandığı `.n3joint` / `.n3anim` bağlantıları
- ilgili body/armor parçaları
- texture dosyaları
- tek bir silah/equipment dosyası

Bu dosyalar public GitHub deposuna commit edilmez. `.gitignore` tarafından dışarıda tutulur.

İlk doğrulama sırası:

1. Kaynak dosyaların SHA-256 özeti çıkarılır.
2. Kaynak dosyalarda değişiklik yapılmadığı doğrulanır.
3. OpenKO-blender ile karakter eksiksiz açılır.
4. `tools/ko_to_unity/export_character.py` ile ayrı FBX üretilir.
5. FBX Unity tarafına alınır.
6. Model, skeleton, texture, idle/walk/run/attack ve silah bağlantısı kontrol edilir.
7. Android debug APK üretilir.

Kaynak KO dosyaları asla dönüştürme sırasında üzerine yazılmaz.
