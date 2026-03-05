# 📁 ZeusDB — EmareOracle — Dosya Yapısı

> **Oluşturulma:** Otomatik  
> **Amaç:** Yapay zekalar kod yazmadan önce mevcut dosya yapısını incelemeli

---

## Proje Dosya Ağacı

```
/Users/emre/Desktop/Emare/emaredatabase
├── EMARE_AI_COLLECTIVE.md
├── EMARE_ORTAK_CALISMA -> /Users/emre/Desktop/Emare/EMARE_ORTAK_CALISMA
├── EMARE_ORTAK_HAFIZA.md
├── data
├── emareoracle_hafiza.md
├── include
│   └── zeusdb.h
├── src
│   ├── btree.c
│   ├── executor.c
│   ├── pager.c
│   ├── parser.c
│   ├── table.c
│   ├── tokenizer.c
│   ├── txn.c
│   └── wal.c
└── tests

6 directories, 12 files

```

---

## 📌 Kullanım Talimatları (AI İçin)

Bu dosya, kod üretmeden önce projenin mevcut yapısını kontrol etmek içindir:

1. **Yeni dosya oluşturmadan önce:** Bu ağaçta benzer bir dosya var mı kontrol et
2. **Yeni klasör oluşturmadan önce:** Mevcut klasör yapısına uygun mu kontrol et
3. **Import/require yapmadan önce:** Dosya yolu doğru mu kontrol et
4. **Kod kopyalamadan önce:** Aynı fonksiyon başka dosyada var mı kontrol et

**Örnek:**
- ❌ "Yeni bir auth.py oluşturalım" → ✅ Kontrol et, zaten `app/auth.py` var mı?
- ❌ "config/ klasörü oluşturalım" → ✅ Kontrol et, zaten `config/` var mı?
- ❌ `from utils import helper` → ✅ Kontrol et, `utils/helper.py` gerçekten var mı?

---

**Not:** Bu dosya otomatik oluşturulmuştur. Proje yapısı değiştikçe güncellenmelidir.

```bash
# Güncelleme komutu
python3 /Users/emre/Desktop/Emare/create_dosya_yapisi.py
```
