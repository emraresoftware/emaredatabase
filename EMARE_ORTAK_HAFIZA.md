# 🧠 Emare Ekosistemi — Ortak Hafıza Dosyası

> **Son Güncelleme:** 4 Mart 2026  
> **Versiyon:** v1.0.0  
> **Kapsam:** Tüm Emare projeleri için paylaşılan bilgi tabanı  
> **Bu dosya tüm projelerde referans alınan ortak hafızadır. Kararlar, standartlar, altyapı ve ekosistem bilgisi burada korunur.**

---

## 📋 İÇİNDEKİLER

1. [Emare Nedir?](#1-emare-nedir)
2. [Proje Envanteri](#2-proje-envanteri)
3. [Sunucu & Altyapı Bilgileri](#3-sunucu--altyapı-bilgileri)
4. [Ortak Teknoloji Standartları](#4-ortak-teknoloji-standartları)
5. [Geliştirme Kuralları](#5-geliştirme-kuralları)
6. [Ortak Mimari Kararlar](#6-ortak-mimari-kararlar)
7. [Güvenlik Standartları](#7-güvenlik-standartları)
8. [Deploy & CI/CD](#8-deploy--cicd)
9. [Emare Hub — Merkezi Yönetim](#9-emare-hub--merkezi-yönetim)
10. [Projeler Arası Bağımlılıklar](#10-projeler-arası-bağımlılıklar)
11. [Gelecek Hedefler](#11-gelecek-hedefler)

---

## 1. Emare Nedir?

**Emare**, tek bir kişi (Emre) tarafından yönetilen, birden fazla SaaS ürün, otomasyon aracı ve altyapı yazılımından oluşan tam yığın bir yazılım ekosistemidir.

### Vizyon
- Her projenin kendi hafızası + paylaşılan ortak hafıza ile çalışması
- AI destekli otonom geliştirme (EmareSetup, EmareHup/DevM)
- Merkezi yönetim paneli (Emare Hub) üzerinden tüm projeleri yönetme
- Gelir üreten SaaS ürünler (Finance, Cloud, Asistan) + araç katmanı

### Genel İlkeler
- **Sıfırdan inşa et:** Mümkün olduğunda third-party bağımlılığı azalt
- **Hafıza dosyaları:** Her proje kendi bağlamını `.md` dosyasında tutar
- **Ortak akıl:** Tüm projeler bu dosyayı referans alır
- **AI-first:** Gemini AI ve OpenAI tüm projelerde birincil yardımcıdır

---

## 2. Proje Envanteri

| # | Proje | İkon | Durum | Kategori | Teknoloji | Yol |
|---|-------|------|-------|----------|-----------|-----|
| 1 | **Emare Asistan** | 🤖 | Production | SaaS Platform | FastAPI, Python, Gemini AI, WhatsApp Bridge, Docker | `/Users/emre/Desktop/Emare/emareasistan` |
| 2 | **EmareCloud** | ☁️ | Production | Infrastructure | Flask, Python, SQLite, SocketIO, Paramiko, xterm.js | `/Users/emre/Desktop/Emare/emarecloud` |
| 3 | **Emare Finance** | 💰 | Production | SaaS Platform | Laravel 12, PHP 8.4, MariaDB, Tailwind CSS, Alpine.js | `/Users/emre/Desktop/Emare/Emare Finance` |
| 4 | **Emare POS / Adisyon** | 🍽️ | Development | POS | Laravel 12, PHP 8.2, SQLite, Alpine.js | `/Users/emre/Desktop/adisyon sistemi/pos-system` |
| 5 | **EmareDesk** | 🖥️ | Ready | Tool | Python, WebSocket, Pillow, mss, pyautogui | `/Users/emre/Desktop/emare desk/remote-desktop` |
| 6 | **EmareSetup** | 🏭 | Development | Infrastructure | Python, FastAPI, React 19, Gemini, OpenAI, SQLAlchemy | `/Users/emre/Desktop/Emare/emaresetup` |
| 7 | **EmareHup** | 🧠 | Development | Infrastructure | Python, Node.js, Gemini, Copilot, LangGraph | `/Users/emre/Desktop/Emare/EmareHup` |
| 8 | **Emarebot** | 🛍️ | Production | Automation | Python 3.12, Tkinter, Trendyol API, Gemini AI | `/Users/emre/Desktop/trendyol_bot_kozmo` |
| 9 | **ZeusDB / EmareOracle** | 🗄️ | Development | Core Engine | C (C11), B+Tree, WAL, ACID, pthread | `/Users/emre/Desktop/oracle` |
| 10 | **SiberEmare** | 🛡️ | Development | Security | Python 3.11, LangGraph, Claude 3.5, GPT-4o, Neo4j | `/Users/emre/Desktop/SiberEmare` |
| 11 | **Emare Log** | 📡 | Development | SaaS Platform | Laravel 12, PHP 8.2, Bootstrap 5, Chart.js, DataTables | `/Users/emre/Desktop/Emare/Emare Log/logmanager` |
| 12 | **Emare Makale** | 📝 | Production | Tool | Python 3.9, Flask 3.0, SQLite, OpenAI gpt-4o | `/Users/emre/Desktop/makale` |
| 13 | **Hive Coordinator** | 👥 | Ready | Infrastructure | Python 3.11, FastAPI, PostgreSQL 16, Redis 7, Celery | `/Users/emre/Desktop/yazılım ekibi/koordinasyon-sistemi` |
| 14 | **Emare Team** | 🏢 | Production | Tool | Flask, SQLite, Vanilla JS, Tailwind CSS | `/Users/emre/Desktop/ekip-yonetici` |
| 15 | **Emare Katip** | 📋 | Ready | Tool | Python, Flask, pytest, GitPython | `/Users/emre/Desktop/Emare/emarekatip` |
| 16 | **Emare Ulak** | 🔌 | Development | Tool | Node.js, Express.js, WebSocket, SQLite, Chrome Extension | `/Users/emre/Desktop/Emare/emareulak` |
| 17 | **Emare Ads** | 📢 | Development | Tool | TypeScript, React, Chrome Extension API, FastAPI | `/Users/emre/Desktop/Emare/emareads` |
| 18 | **Emare AI** | 🤖 | Development | Core Engine | PyTorch, LLaMA/Mistral, FastAPI, vLLM/Ollama | `/Users/emre/Desktop/Emare/emareai` |

### Durum Açıklamaları
- **Production** → Canlıda çalışıyor, gerçek kullanıcı var (6 proje)
- **Development** → Aktif geliştirme aşamasında (8 proje)
- **Ready** → Tamamlandı, deploy bekleniyor (4 proje)

**Toplam:** 18 proje

---

## 3. Sunucu & Altyapı Bilgileri

### Aktif Sunucular

| Sunucu | IP | Kullanım | Projeler |
|--------|----|----------|----------|
| **Ana Sunucu** | `77.92.152.3` | Emare Asistan API + Finance Web | Asistan (8000), Finance (3000) |
| **EmareCloud Sunucu** | `185.189.54.104` | EmareCloud paneli | EmareCloud (80) |

### Port Haritası (Lokal Geliştirme)

| Port | Proje |
|------|-------|
| 5555 | EmareCloud (lokal) + **Emare Hub** |
| 8000 | EmareSetup API / Emare Asistan (prod) |
| 8001 | Hive Coordinator API |
| 8080 | Emare POS / EmareDesk |
| 8082 | Emare Log |
| 3000 | Emare Finance |
| 5000 | Emare Makale |
| 5050 | Emare Team |

### Domain & DNS
- EmareCloud → Cloudflare ile yönetiliyor
- Tüm sunucular → Ubuntu Server

---

## 4. Ortak Teknoloji Standartları

### Backend
| Dil/Framework | Projeler |
|---|---|
| **Python / Flask** | EmareCloud, Emare Makale, Emare Team, Emare Hub |
| **Python / FastAPI** | Emare Asistan, EmareSetup, Hive Coordinator |
| **PHP / Laravel 12** | Emare Finance, Emare POS, Emare Log |
| **Python (standalone)** | EmareDesk, Emarebot, SiberEmare |
| **C (C11)** | ZeusDB / EmareOracle |

### Frontend
| Teknoloji | Kullanım |
|---|---|
| **Tailwind CSS** | Emare Finance, Emare Hub dashboard |
| **Alpine.js** | Emare Finance, Emare POS, Emare Hub |
| **Vanilla JS** | EmareCloud, Emare Team, EmareDesk |
| **React 19** | EmareSetup (frontend) |
| **xterm.js** | EmareCloud (web terminal) |

### Veritabanı
| DB | Projeler |
|---|---|
| **SQLite** | EmareCloud, Emare POS, Emare Makale, Emare Team |
| **MariaDB / MySQL** | Emare Finance |
| **PostgreSQL 16** | Hive Coordinator |
| **Neo4j** | SiberEmare (graph DB) |

### AI & LLM
| Model | Projeler |
|---|---|
| **Gemini AI (Google)** | Emare Asistan (birincil), EmareSetup, EmareHup, Emarebot |
| **OpenAI GPT-4o** | Emare Makale, SiberEmare |
| **Claude 3.5 Sonnet** | SiberEmare |
| **LangGraph** | SiberEmare, EmareHup/DevM |

---

## 5. Geliştirme Kuralları

### Python Projeleri
```
- Python 3.9+ (3.11 tercih)
- type | None sözdizimi YOK — Python 3.9 uyumlu olacak şekilde Optional[X]
- venv veya .venv klasörü proje içinde
- requirements.txt zorunlu
- app.py veya main.py giriş noktası
```

### PHP / Laravel Projeleri
```
- Laravel 12, PHP 8.2+
- Blade şablonlar
- Artisan CLI ile migration
- .env ile yapılandırma
- composer.json zorunlu
```

### Genel
```
- Her proje kök dizininde hafıza .md dosyası bulunur
- README.md tüm projelerde olmalı
- Git ile versiyon kontrolü
- .env dosyası .gitignore'da
- Port çakışması olmayacak şekilde portlar atanmış (bkz. Port Haritası)
```

### Naming Convention
```
- Python dosyaları: snake_case.py
- PHP sınıfları: PascalCase
- JS değişkenleri: camelCase
- Proje klasörleri: küçük harf, tire ile
```

---

## 6. Ortak Mimari Kararlar

### Hafıza Sistemi
```
Tüm projeler → Kendi hafiza.md dosyası
Tüm projeler → EMARE_ORTAK_HAFIZA.md (bu dosya) referans alır
Emare Hub → projects.json ile tüm projeleri yönetir
hub_autodetect.py → Yeni projeleri otomatik algılar
```

### Modulerlik
- Her proje bağımsız çalışabilmeli
- Projeler arası doğrudan import YOK — HTTP API ile iletişim
- Ortak kodlar kopyalanır, merkezi lib paketi kullanılmaz (bağımsızlık)

### Deployment
```
Production → Sunucu üzerinde doğrudan Python/PHP ile çalıştırma
Konteyner → EmareCloud sunucusu için Docker Compose
Reverse Proxy → Nginx (EmareCloud, Finance)
Süreç Yönetimi → Supervisor (EmareCloud)
```

### Loglama
- Python → `logging` + dosyaya yaz
- Laravel → `storage/logs/laravel.log`
- Emare Hub → `/tmp/hub.log` veya terminale

---

## 7. Güvenlik Standartları

### API
- JWT veya Session tabanlı kimlik doğrulama (proje bağlı)
- CORS sadece gerekli domainlere açık
- Rate limiting (production'da)
- HTTPS zorunlu (production)

### Sunucu
- SSH key tabanlı giriş
- UFW firewall aktif (EmareCloud üzerinden yönetilir)
- Root login kapalı
- Fail2ban aktif

### Kod
- `.env` dosyaları asla commit edilmez
- API anahtarları ortam değişkenlerinde
- SQL injection → ORM kullanım (SQLAlchemy, Eloquent)
- XSS → şablon escape (Jinja2, Blade)

---

## 8. Deploy & CI/CD

### EmareCloud Sunucusu (185.189.54.104)
```bash
ssh root@185.189.54.104
cd /root/emarecloud
git pull origin main
supervisorctl restart emarecloud
```

### Ana Sunucu (77.92.152.3)
```bash
ssh root@77.92.152.3
# Asistan
cd /root/asistan && git pull && supervisorctl restart asistan
# Finance
cd /root/finance && git pull && php artisan migrate --force
```

### Lokal Geliştirme
```bash
# Emare Hub başlat
cd /Users/emre/Desktop/Emare
./start_hub.sh
# http://127.0.0.1:5555

# Hub CLI
python hub_cli.py list
python hub_cli.py ping
python hub_cli.py start <proje-id>
```

---

## 9. Emare Hub — Merkezi Yönetim

**Konum:** `/Users/emre/Desktop/Emare/`  
**URL:** `http://127.0.0.1:5555`  
**Başlatma:** `./start_hub.sh`

### Dosyalar
| Dosya | Açıklama |
|---|---|
| `hub.py` | Flask backend, 15+ API endpoint |
| `hub_cli.py` | Rich terminal CLI |
| `hub_templates/index.html` | Dark theme web dashboard |
| `hub_autodetect.py` | Otomatik proje algılama motoru |
| `projects.json` | Tüm projelerin yapılandırma kayıt dosyası |
| `start_hub.sh` | Tek komutla başlatma script'i |
| `.hub_venv/` | Emare Hub Python ortamı |
| `.hub_pids.json` | Çalışan proje PID'leri |

### API Endpoint'leri
```
GET  /api/projects          → Tüm projeler
POST /api/projects          → Yeni proje ekle
PUT  /api/projects/<id>     → Proje güncelle
DELETE /api/projects/<id>   → Proje sil
GET  /api/ping/<id>         → Sunucu ping
GET  /api/ping-all          → Tüm sunucuları ping
GET  /api/git/<id>          → Git durumu
GET  /api/git-all           → Tüm projelerin git durumu
POST /api/start/<id>        → Proje başlat
POST /api/stop/<id>         → Proje durdur
GET  /api/running           → Çalışan prosesler
GET  /api/memory/<id>       → Hafıza dosyası oku
GET  /api/notes/<id>        → Notlar
POST /api/notes/<id>        → Not ekle
GET  /api/stats             → Özet istatistikler
GET  /api/browse            → macOS klasör seçici
POST /api/detect            → Klasörü tara, proje bilgilerini çıkar
POST /api/vscode-sync       → VS Code workspace güncelle
POST /api/open-vscode/<id>  → Projeyi VS Code'da aç
```

---

## 10. Projeler Arası Bağımlılıklar

```
Emare Asistan
  └─ Gemini AI API (harici)
  └─ WhatsApp Bridge (ayrı servis)

EmareCloud
  └─ EmareToken / Blockchain modülü (dahili)
  └─ LXD (sunucu üzerinde)

Emare Finance
  └─ E-Fatura API (harici)
  └─ SMS API (harici)

EmareSetup
  └─ Gemini AI / OpenAI API (harici)
  └─ ZeusDB → ileride kendi DB motoru kullanabilir

EmareHup / DevM
  └─ VS Code Copilot (harici)
  └─ LangGraph (harici)
  └─ EmareSetup ile entegrasyon planlanıyor

SiberEmare
  └─ Neo4j (lokal/sunucu)
  └─ PGVector (PostgreSQL eklentisi)
  └─ Claude 3.5 + GPT-4o (harici)

Hive Coordinator
  └─ PostgreSQL 16 (lokal)
  └─ Redis 7 (lokal)

Emare Hub (bu sistem)
  └─ Tüm projeleri yönetir, bağımlılık yok
```

---

## 11. Gelecek Hedefler

### Kısa Vadeli
- [ ] EmareSetup → EmareHup/DevM ile tam entegrasyon
- [ ] ZeusDB → ilk SQL parser (SELECT * FROM test)  
- [ ] Emare POS → production deployment
- [ ] SiberEmare → .env doldur, ilk pentest raporu üret
- [ ] Emare Log → ilk tenant kaydı ve MikroTik bağlantısı

### Orta Vadeli
- [ ] Emare Finance → v2 migrasi (multi-tenant kira modeli)
- [ ] EmareCloud → v2 Kubernetes desteği
- [ ] Emare Hub → mobil uygulama (Flutter)
- [ ] Tüm projeler → merkezi loglama sistemi

### Uzun Vadeli
- [ ] ZeusDB → Emare ekosisteminin kendi veritabanı motoru haline gelsin
- [ ] EmareSetup → tam otonom yazılım geliştirme fabrikası
- [ ] EmareCloud → halka açık SaaS ürün

---

*Bu dosya Emare Hub tarafından otomatik olarak senkronize edilebilir.*  
*Bireysel proje hafızaları için ilgili proje klasörüne bakınız.*
