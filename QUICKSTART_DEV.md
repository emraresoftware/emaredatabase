# Emare Database — Hızlı Başlangıç (Developer Guide)

> İlk kez bu projeye dokunuyorsan bu belgeyi oku.
> 5 dakikada hazır ol.

---

## 1. Ön Koşullar

- Python 3.10+ (`python3 --version`)
- Git (`git --version`)
- pip3 (`pip3 --version`)

---

## 2. Kurulum

```bash
# Repo'ya gir
cd /Users/emre/Desktop/Emare/emaredatabase

# Kurulumu çalıştır
chmod +x setup.sh && ./setup.sh
```

Bu komut:
- ✅ Sanal ortam (.venv) oluşturur
- ✅ Bağımlılıkları yükler
- ✅ .env dosyasını kopyalar
- ✅ data/ klasörünü oluşturur

---

## 3. Çalıştırma

```bash
# Başlat
./start.sh

# veya manuel
source .venv/bin/activate
python3 main.py
```

Port: **8000**

---

## 4. ⚠️ Kodlamadan Önce Oku

**ZORUNLU** dosyalar (sırasıyla):

| # | Dosya | Ne Öğrenirsin |
|---|-------|---------------|
| 1 | `EMARE_ORTAK_HAFIZA.md` | Ekosistem bilgisi |
| 2 | `EMARE_ANAYASA.md` | Kodlama kuralları |
| 3 | `EMARE_AI_COLLECTIVE.md` | AI deneyimleri |
| 4 | `DERVISHIN_CEYIZI.md` | Rol hiyerarşisi (RBAC) |
| 5 | `DOSYA_YAPISI.md` | Dosya yapısı |
| 6 | `emaredatabase_HAFIZA.md` | Proje geçmişi |

---

## 5. Proje Yapısı (Kısa)

```
emaredatabase/
├── main.py                    # Giriş noktası
├── requirements.txt           # Bağımlılıklar
├── setup.sh / start.sh        # Kurulum & başlatma
├── DERVISHIN_CEYIZI.md        # RBAC ağacı (olmazsa olmaz)
├── EMARE_ANAYASA.md           # AI kuralları
├── EMARE_AI_COLLECTIVE.md     # AI deneyimleri
├── EMARE_ORTAK_HAFIZA.md      # Ekosistem hafızası
├── web_dizayn/                # Tasarım rehberi + landing page
└── EMARE_ORTAK_CALISMA/       # Paylaşılan dosyalar (symlink)
```

---

## 6. Sık Kullanılan Komutlar

```bash
# Test çalıştır
python3 -m pytest tests/

# Lint
python3 -m ruff check .

# Health check (sunucu çalışıyorsa)
curl http://localhost:8000/health
```

---

## 7. Yardım

- **Derviş**: EmareAPI'de `emaredatabase Dervishi` bu projeden sorumlu
- **Dergah**: Diğer projelerle iletişim için `emareapi/Dergah/`
- **Hub**: Dashboard'dan tüm projeleri gör (`emare_dashboard/`)

---

*emarework Dervishi — Çeyiz Sistemi v2.0*
