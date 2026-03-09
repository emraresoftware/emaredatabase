#!/bin/bash
# Emare Database — Kurulum Scripti
# Çeyiz Tarihi: 06 March 2026

echo "🔧 Emare Database kurulumu başlatılıyor..."

# Python sanal ortam
if [ ! -d ".venv" ]; then
    python3 -m venv .venv
    echo "✓ Sanal ortam oluşturuldu"
fi

source .venv/bin/activate

# Bağımlılıklar
if [ -f "requirements.txt" ]; then
    pip install -r requirements.txt
    echo "✓ Python bağımlılıkları yüklendi"
fi

# Node bağımlılıkları (varsa)
if [ -f "package.json" ]; then
    npm install
    echo "✓ Node.js bağımlılıkları yüklendi"
fi

# Data klasörü
mkdir -p data

# .env dosyası
if [ ! -f ".env" ] && [ -f ".env.example" ]; then
    cp .env.example .env
    echo "✓ .env dosyası oluşturuldu"
fi

echo ""
echo "✅ Emare Database kurulumu tamamlandı!"
echo "   Başlatmak için: ./start.sh"
