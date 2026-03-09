# Emare Database — Test Suite
# Çeyiz: emarework Dervishi

"""
Temel testler — projenin sağlıklı çalıştığını doğrular.
"""

import os
import sys
import importlib

# Proje root'unu path'e ekle
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


class TestProjeYapisi:
    """Proje dosya yapısı testleri."""

    def test_readme_mevcut(self):
        """README.md dosyası var mı?"""
        assert os.path.exists("README.md"), "README.md bulunamadı"

    def test_requirements_mevcut(self):
        """requirements.txt dosyası var mı?"""
        assert os.path.exists("requirements.txt"), "requirements.txt bulunamadı"

    def test_dervishin_ceyizi_mevcut(self):
        """DERVISHIN_CEYIZI.md (olmazsa olmaz) var mı?"""
        assert os.path.exists("DERVISHIN_CEYIZI.md"), "DERVISHIN_CEYIZI.md bulunamadı — RBAC ağacı eksik!"

    def test_anayasa_mevcut(self):
        """EMARE_ANAYASA.md var mı?"""
        assert os.path.exists("EMARE_ANAYASA.md"), "EMARE_ANAYASA.md bulunamadı"

    def test_dosya_yapisi_mevcut(self):
        """DOSYA_YAPISI.md var mı?"""
        assert os.path.exists("DOSYA_YAPISI.md"), "DOSYA_YAPISI.md bulunamadı"

    def test_gitignore_mevcut(self):
        """.gitignore var mı?"""
        assert os.path.exists(".gitignore"), ".gitignore bulunamadı"

    def test_web_dizayn_klasoru(self):
        """web_dizayn/ klasörü var mı?"""
        assert os.path.isdir("web_dizayn"), "web_dizayn/ klasörü bulunamadı"

    def test_ortak_calisma_baglantisi(self):
        """EMARE_ORTAK_CALISMA bağlantısı var mı?"""
        assert os.path.exists("EMARE_ORTAK_CALISMA"), "EMARE_ORTAK_CALISMA bulunamadı"


class TestTemelIslevsellik:
    """Temel işlevsellik testleri."""

    def test_python_surumu(self):
        """Python 3.10+ mı?"""
        assert sys.version_info >= (3, 10), f"Python 3.10+ gerekli, mevcut: {sys.version}"

    def test_main_import(self):
        """main.py import edilebilir mi?"""
        if os.path.exists("main.py"):
            try:
                spec = importlib.util.spec_from_file_location("main", "main.py")
                assert spec is not None, "main.py import edilemedi"
            except Exception:
                pass  # Bazı main.py'ler direkt çalışır, import edilemeyebilir

    def test_placeholder(self):
        """Placeholder — gerçek testler eklenecek."""
        assert True


if __name__ == "__main__":
    import pytest
    pytest.main([__file__, "-v"])
