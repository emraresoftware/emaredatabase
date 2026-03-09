# 🏛️ EMARE ANAYASA — Yapay Zeka Kodlama Kuralları

> **Yürürlük Tarihi:** 06 March 2026
> **Kapsam:** Emare Database projesindeki tüm yapay zekalar
> **Amaç:** Tutarlı, kaliteli ve sürdürülebilir kod üretimi
> **Bağlayıcılık:** Bu anayasadaki tüm maddeler **zorunludur**

---

## 📜 Temel İlkeler

Emare Database projesinde kod yazan her yapay zeka bu anayasaya uymak zorundadır.

---

## Madde 1: Ortak Hafıza Zorunluluğu

**Kod yazmaya başlamadan önce MUTLAKA ortak hafızayı oku.**

```markdown
📖 ZORUNLU OKUMA LİSTESİ (Kod yazmadan önce):
1. EMARE_ORTAK_HAFIZA.md (ekosistem envanteri)
2. EMARE_AI_COLLECTIVE.md (AI deneyimleri)
3. DOSYA_YAPISI.md (mevcut dosya yapısı)
4. emaredatabase_HAFIZA.md (proje hafızası)
5. DERVISHIN_CEYIZI.md (rol ve yetki ağacı)
```

**Yaptırım:**
- ❌ Bu maddeye uymayan AI'nın yazdığı kod kabul edilmez

---

## Madde 2: Mevcut Kod Kontrolü

**Yeni bir dosya/fonksiyon yazmadan önce benzerinin var olup olmadığını kontrol et.**

### 2.1. Dosya Kontrolü
```bash
1. DOSYA_YAPISI.md'yi kontrol et
2. Benzer isimli dosya var mı bak
3. İlgili modülün içini incele
```

### 2.2. Fonksiyon/Sınıf Kontrolü
```bash
1. Proje içinde grep veya semantic search yap
2. Aynı işi yapan kod var mı kontrol et
3. Varsa onu kullan veya refactor et
```

### 2.3. Kütüphane Kontrolü
```bash
1. requirements.txt / package.json kontrol et
2. Benzer kütüphane zaten var mı bak
3. EMARE_AI_COLLECTIVE.md'de önerilen kütüphanelere bak
```

---

## Madde 3: Dosya & Kod Standartları

### 3.1. Python
- Python 3.10+ (f-string, type hint zorunlu)
- Encoding: UTF-8 (Türkçe karakter desteği)
- Docstring: Google style
- Import sırası: stdlib → third-party → local
- Max satır uzunluğu: 120 karakter

### 3.2. Dosya İsimlendirme
- Python: `snake_case.py`
- JavaScript/TypeScript: `camelCase.ts` veya `PascalCase.tsx`
- CSS: `kebab-case.css`
- Markdown: `BUYUK_HARF.md` (önemli belgeler), `kucuk_harf.md` (genel)

### 3.3. Commit Mesajları
```
feat: yeni özellik eklendi
fix: hata düzeltildi
docs: dokümantasyon güncellendi
refactor: kod yeniden yapılandırıldı
test: test eklendi/düzeltildi
chore: build/config değişikliği
```

---

## Madde 4: Güvenlik Kuralları

1. **Asla** `.env` dosyasını commit etme
2. **Asla** şifre/token'ı hardcode etme
3. **Her zaman** input validation yap
4. **Her zaman** RBAC kontrol et (DERVISHIN_CEYIZI.md'ye bak)
5. SQL injection'a karşı: ORM veya parametreli query kullan
6. XSS'e karşı: Jinja2 auto-escape aktif
7. CORS: Production'da `*` kullanma

---

## Madde 5: Test Zorunluluğu

- Her yeni modül için en az 1 test dosyası
- CI/CD pipeline'da testler geçmeli
- Coverage hedefi: %70+

---

## Madde 6: Hafıza Güncelleme

Her büyük değişiklikte şu dosyaları güncelle:
1. `emaredatabase_HAFIZA.md` — Ne yapıldı, ne öğrenildi
2. `DOSYA_YAPISI.md` — Yeni dosya eklendiyse
3. `COPILOT_SESSION_HAFIZA.md` — Session bazlı ilerleme

---

*Bu anayasa Emare ekosisteminin ortak kurallarından türetilmiştir.*
*emarework Dervishi — Çeyiz Sistemi v2.0*
