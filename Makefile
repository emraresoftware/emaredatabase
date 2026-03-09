# Emare Database — Makefile
# Çeyiz: emarework Dervishi

.PHONY: help setup run test lint clean docker

help: ## Bu yardım mesajını göster
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2}'

setup: ## Kurulum — venv + dependencies
	@chmod +x setup.sh && ./setup.sh

run: ## Uygulamayı başlat
	@chmod +x start.sh && ./start.sh

test: ## Testleri çalıştır
	@source .venv/bin/activate 2>/dev/null; python3 -m pytest tests/ -v

lint: ## Lint kontrolü
	@source .venv/bin/activate 2>/dev/null; python3 -m ruff check . 2>/dev/null || echo "ruff yüklü değil: pip install ruff"

clean: ## Geçici dosyaları temizle
	@find . -type d -name __pycache__ -exec rm -rf {} + 2>/dev/null
	@find . -name "*.pyc" -delete 2>/dev/null
	@rm -rf .pytest_cache .ruff_cache .coverage htmlcov dist build *.egg-info
	@echo "✓ Temizlendi"

docker: ## Docker ile çalıştır
	@docker-compose up --build -d

docker-down: ## Docker durdur
	@docker-compose down

health: ## Health check
	@curl -s http://localhost:8000/health 2>/dev/null | python3 -m json.tool || echo "Sunucu çalışmıyor"
