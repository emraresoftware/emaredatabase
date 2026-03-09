#!/usr/bin/env python3
"""
Emare Derviş Haberleşme Sistemi (GitHub Issue tabanlı)
======================================================
Dervişler arası anlık mesajlaşma — GitHub Issues API üzerinden.

Kullanım:
  from emare_messenger import EmareMesaj

  mesaj = EmareMesaj("emarecloud")          # Gönderen derviş adı
  mesaj.gonder("emaresetup", "Deployment tamam, API hazır!")
  mesaj.gonder_herkese("v2.0 yayınlandı!")
  inbox = mesaj.oku()                       # Gelen mesajları oku
  mesaj.yanit(issue_no, "Teşekkürler, aldım!")
  
CLI:
  python emare_messenger.py gonder emaresetup "API hazır"
  python emare_messenger.py herkese "Güncelleme yapıldı"
  python emare_messenger.py oku
  python emare_messenger.py yanit 5 "Tamam aldım"
"""

import urllib.request
import json
import os
import sys
from datetime import datetime

# ── Yapılandırma ──
def _load_token():
    """Token'ı ortam değişkeni veya .github_token dosyasından oku."""
    # 1. Ortam değişkeni
    token = os.getenv("GITHUB_TOKEN", "")
    if token:
        return token
    # 2. EMARE_ORTAK_CALISMA/.github_token dosyası
    token_paths = [
        os.path.join(os.path.dirname(os.path.abspath(__file__)), ".github_token"),
        os.path.join(os.path.dirname(os.path.abspath(__file__)), "EMARE_ORTAK_CALISMA", ".github_token"),
    ]
    for path in token_paths:
        if os.path.exists(path):
            with open(path, "r") as f:
                token = f.read().strip()
                if token:
                    return token
    return ""

GITHUB_TOKEN = _load_token()
GITHUB_USER = os.getenv("GITHUB_USER", "emraresoftware")
MSG_REPO = "emare-ortak-calisma"   # Haberleşme kanalı olarak kullanılan repo
API_BASE = f"https://api.github.com/repos/{GITHUB_USER}/{MSG_REPO}"

# Mesaj etiketleri
LABEL_MSG = "dervis-mesaj"
LABEL_BROADCAST = "duyuru"
LABEL_URGENT = "acil"


class EmareMesaj:
    """Dervişler arası GitHub Issue tabanlı haberleşme."""

    def __init__(self, dervis_adi: str):
        self.dervis = dervis_adi
        self._labels_kuruldu = False

    # ── API Yardımcıları ──

    def _api(self, method: str, endpoint: str, data: dict = None) -> tuple:
        """GitHub API çağrısı."""
        url = f"{API_BASE}{endpoint}"
        headers = {
            "Authorization": f"token {GITHUB_TOKEN}",
            "Accept": "application/vnd.github.v3+json",
            "Content-Type": "application/json",
        }
        body = json.dumps(data).encode() if data else None
        req = urllib.request.Request(url, data=body, headers=headers, method=method)
        try:
            with urllib.request.urlopen(req) as resp:
                return resp.status, json.loads(resp.read().decode())
        except urllib.error.HTTPError as e:
            try:
                return e.code, json.loads(e.read().decode())
            except Exception:
                return e.code, {"message": str(e)}

    def _ensure_labels(self):
        """Gerekli etiketleri oluştur (bir kere)."""
        if self._labels_kuruldu:
            return
        labels = [
            (LABEL_MSG, "0075ca", "Derviş arası mesaj"),
            (LABEL_BROADCAST, "e4e669", "Tüm dervişlere duyuru"),
            (LABEL_URGENT, "d93f0b", "Acil mesaj"),
        ]
        for name, color, desc in labels:
            self._api("POST", "/labels", {
                "name": name,
                "color": color,
                "description": desc,
            })
        self._labels_kuruldu = True

    # ── Mesaj Gönderme ──

    def gonder(self, alici: str, mesaj: str, acil: bool = False) -> dict:
        """Belirli bir dervişe mesaj gönder."""
        self._ensure_labels()
        zaman = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        labels = [LABEL_MSG, f"alici:{alici}", f"gonderen:{self.dervis}"]
        if acil:
            labels.append(LABEL_URGENT)

        title = f"📨 [{self.dervis}] → [{alici}]: {mesaj[:60]}"
        body = f"""## Derviş Mesajı

| Alan | Değer |
|------|-------|
| 📤 Gönderen | `{self.dervis}` |
| 📥 Alıcı | `{alici}` |
| 🕐 Zaman | {zaman} |
| {'🔴 ACİL' if acil else '🔵 Normal'} | Öncelik |

---

### Mesaj:
{mesaj}

---
*Bu mesaj Emare Derviş Haberleşme Sistemi tarafından otomatik oluşturulmuştur.*
"""
        status, resp = self._api("POST", "/issues", {
            "title": title,
            "body": body,
            "labels": labels,
        })
        if status == 201:
            print(f"  ✅ Mesaj gönderildi → {alici} (#{resp['number']})")
            return resp
        else:
            print(f"  ❌ Gönderilemedi: {resp.get('message', '')}")
            return resp

    def gonder_herkese(self, mesaj: str, acil: bool = False) -> dict:
        """Tüm dervişlere duyuru yap (broadcast)."""
        self._ensure_labels()
        zaman = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        labels = [LABEL_MSG, LABEL_BROADCAST, f"gonderen:{self.dervis}"]
        if acil:
            labels.append(LABEL_URGENT)

        title = f"📢 [{self.dervis}] → [HERKES]: {mesaj[:60]}"
        body = f"""## 📢 Duyuru — Tüm Dervişlere

| Alan | Değer |
|------|-------|
| 📤 Gönderen | `{self.dervis}` |
| 📥 Alıcı | **TÜM DERVİŞLER** |
| 🕐 Zaman | {zaman} |
| {'🔴 ACİL' if acil else '🟢 Duyuru'} | Tür |

---

### Mesaj:
{mesaj}

---
*Bu duyuru Emare Derviş Haberleşme Sistemi tarafından otomatik oluşturulmuştur.*
"""
        status, resp = self._api("POST", "/issues", {
            "title": title,
            "body": body,
            "labels": labels,
        })
        if status == 201:
            print(f"  📢 Duyuru yayınlandı (#{resp['number']})")
            return resp
        else:
            print(f"  ❌ Duyuru gönderilemedi: {resp.get('message', '')}")
            return resp

    # ── Mesaj Okuma ──

    def oku(self, sadece_okunmamis: bool = True, limit: int = 20) -> list:
        """Bu dervişe gelen mesajları oku."""
        # Doğrudan bu dervişe gelen + broadcast mesajları
        mesajlar = []

        # Direkt mesajlar
        status, issues = self._api(
            "GET",
            f"/issues?labels={LABEL_MSG},alici:{self.dervis}&state=open&per_page={limit}"
        )
        if status == 200:
            mesajlar.extend(issues)

        # Broadcast mesajlar
        status, broadcasts = self._api(
            "GET",
            f"/issues?labels={LABEL_MSG},{LABEL_BROADCAST}&state=open&per_page={limit}"
        )
        if status == 200:
            # Kendi gönderdiğini hariç tut
            for b in broadcasts:
                gonderen_labels = [l["name"] for l in b.get("labels", []) if l["name"].startswith("gonderen:")]
                if f"gonderen:{self.dervis}" not in gonderen_labels:
                    mesajlar.append(b)

        # Tarihe göre sırala (yeniden eskiye)
        mesajlar.sort(key=lambda x: x.get("created_at", ""), reverse=True)

        if not mesajlar:
            print(f"  📭 {self.dervis}: Yeni mesaj yok")
        else:
            print(f"\n  📬 {self.dervis} — {len(mesajlar)} mesaj:\n")
            for m in mesajlar:
                no = m["number"]
                baslik = m["title"]
                tarih = m["created_at"][:16].replace("T", " ")
                labels = [l["name"] for l in m.get("labels", [])]
                acil = "🔴" if LABEL_URGENT in labels else "  "
                print(f"  {acil} #{no:>3} | {tarih} | {baslik[:70]}")

        return mesajlar

    def tum_mesajlar(self, limit: int = 50) -> list:
        """Tüm derviş mesajlarını listele."""
        status, issues = self._api(
            "GET",
            f"/issues?labels={LABEL_MSG}&state=open&per_page={limit}"
        )
        if status == 200:
            print(f"\n  📋 Toplam {len(issues)} açık mesaj:\n")
            for m in issues:
                no = m["number"]
                baslik = m["title"]
                tarih = m["created_at"][:16].replace("T", " ")
                print(f"  #{no:>3} | {tarih} | {baslik[:70]}")
            return issues
        return []

    # ── Yanıt ──

    def yanit(self, issue_no: int, mesaj: str) -> dict:
        """Bir mesaja yanıt ver (comment)."""
        zaman = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        body = f"""**💬 Yanıt — `{self.dervis}`** ({zaman})

---

{mesaj}
"""
        status, resp = self._api("POST", f"/issues/{issue_no}/comments", {"body": body})
        if status == 201:
            print(f"  💬 Yanıt eklendi → #{issue_no}")
            return resp
        else:
            print(f"  ❌ Yanıt eklenemedi: {resp.get('message', '')}")
            return resp

    # ── Mesaj Durumu ──

    def okundu(self, issue_no: int) -> dict:
        """Mesajı okundu olarak işaretle (kapat)."""
        status, resp = self._api("PATCH", f"/issues/{issue_no}", {"state": "closed"})
        if status == 200:
            print(f"  ✔️  #{issue_no} okundu olarak işaretlendi")
        return resp

    def durum_guncelle(self, issue_no: int, durum: str) -> dict:
        """Mesaja durum etiketi ekle."""
        status, resp = self._api("POST", f"/issues/{issue_no}/labels", {
            "labels": [f"durum:{durum}"]
        })
        return resp


# ── CLI Arayüzü ──

def cli():
    """Komut satırı arayüzü."""
    if len(sys.argv) < 2:
        print("""
Emare Derviş Haberleşme Sistemi
================================
Kullanım:
  python emare_messenger.py <dervis_adi> gonder <alici> <mesaj>
  python emare_messenger.py <dervis_adi> herkese <mesaj>
  python emare_messenger.py <dervis_adi> oku
  python emare_messenger.py <dervis_adi> tumu
  python emare_messenger.py <dervis_adi> yanit <issue_no> <mesaj>
  python emare_messenger.py <dervis_adi> okundu <issue_no>

Örnekler:
  python emare_messenger.py emarecloud gonder emaresetup "API v2 hazır"
  python emare_messenger.py emaresetup oku
  python emare_messenger.py emarecloud herkese "Tüm servisleri güncelleyin"
  python emare_messenger.py emaresetup yanit 3 "Tamam, aldım"
""")
        return

    dervis = sys.argv[1]
    komut = sys.argv[2] if len(sys.argv) > 2 else "oku"
    m = EmareMesaj(dervis)

    if komut == "gonder" and len(sys.argv) >= 5:
        alici = sys.argv[3]
        mesaj_text = " ".join(sys.argv[4:])
        m.gonder(alici, mesaj_text)

    elif komut == "herkese" and len(sys.argv) >= 4:
        mesaj_text = " ".join(sys.argv[3:])
        m.gonder_herkese(mesaj_text)

    elif komut == "oku":
        m.oku()

    elif komut == "tumu":
        m.tum_mesajlar()

    elif komut == "yanit" and len(sys.argv) >= 5:
        issue_no = int(sys.argv[3])
        mesaj_text = " ".join(sys.argv[4:])
        m.yanit(issue_no, mesaj_text)

    elif komut == "okundu" and len(sys.argv) >= 4:
        issue_no = int(sys.argv[3])
        m.okundu(issue_no)

    else:
        print("Bilinmeyen komut. Yardım için parametresiz çalıştırın.")


if __name__ == "__main__":
    cli()
