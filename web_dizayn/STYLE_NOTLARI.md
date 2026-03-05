# Emare Finance — Tasarım Rehberi (Design System)

> POS yazılımı dahil tüm ürünlerde tutarlı görsel dil için referans doküman.
> Tarih: 2 Mart 2026

---

## 1. RENK PALETİ (Brand Colors)

| Token         | HEX       | RGB                | Kullanım                              |
|---------------|-----------|--------------------|---------------------------------------|
| `brand-50`    | `#eef2ff` | `238, 242, 255`    | Çok açık arka plan, hover bg          |
| `brand-100`   | `#e0e7ff` | `224, 231, 255`    | Açık arka plan                        |
| `brand-200`   | `#c7d2fe` | `199, 210, 254`    | Border (secondary buton)              |
| `brand-300`   | `#a5b4fc` | `165, 180, 252`    | İkon accent                           |
| `brand-400`   | `#818cf8` | `129, 140, 248`    | Hover accent                          |
| `brand-500`   | `#6366f1` | `99, 102, 241`     | **Ana marka rengi (Primary)**         |
| `brand-600`   | `#4f46e5` | `79, 70, 229`      | Hover/active primary                  |
| `brand-700`   | `#4338ca` | `67, 56, 202`      | Koyu primary, secondary buton text    |
| `brand-800`   | `#3730a3` | `55, 48, 163`      | Koyu bg accent                        |
| `brand-900`   | `#312e81` | `49, 46, 129`      | Çok koyu bg                           |
| `brand-950`   | `#1e1b4b` | `30, 27, 75`       | En koyu bg (hero, footer)             |

### Ek Koyu Tonlar
- **Hero Fallback:** `#0f0a2e` — `rgb(15, 10, 46)`
- **Footer BG:** Tailwind `gray-950`

---

## 2. GRİ TONLARI (Neutral / Gray)

| Token       | Kullanım                                    |
|-------------|---------------------------------------------|
| `white`     | Sayfa arka planı, kart arka planı           |
| `gray-100`  | Border divider, mobile menu border          |
| `gray-300`  | Footer body text                            |
| `gray-400`  | Footer alt bar text, ikincil bilgi          |
| `gray-500`  | Kart alt açıklamaları                       |
| `gray-600`  | Body paragraph text (ana metin)             |
| `gray-700`  | Mobil menü text, hamburger ikon             |
| `gray-800`  | Body default (`text-gray-800`)              |
| `gray-900`  | Başlıklar, navbar logo text                 |
| `gray-950`  | Footer arka plan                            |

---

## 3. EK RENKLER

| Renk          | HEX       | Kullanım                                     |
|---------------|-----------|----------------------------------------------|
| `purple-600`  | `#9333ea` | Gradient bitiş (buton, logo, CTA)            |
| `green-500`   | `#22c55e` | Başarı/güvenlik ikonları (SSL, KVKK)         |
| `green-400`   | `#4ade80` | Aktif durum badge                            |
| `amber-500`   | `#f59e0b` | Uyarı                                        |
| `red-500`     | `#ef4444` | Hata                                         |

---

## 4. GRADYANLAR

### Primary Buton / Logo Gradient
```css
background: linear-gradient(to right, #6366f1, #9333ea);
/* Tailwind: bg-gradient-to-r from-brand-500 to-purple-600 */
```

### Gradient Text (Başlıklarda)
```css
background: linear-gradient(135deg, #4f46e5, #7c3aed, #6d28d9);
-webkit-background-clip: text;
-webkit-text-fill-color: transparent;
background-clip: text;
```

### Hero / CTA Koyu Arka Plan
```css
background: linear-gradient(-45deg, #0f0a2e, #1e1b4b, #1e1b4b, #312e81);
background-size: 400% 400%;
animation: gradient 8s ease infinite;
```

### Hero Pattern Overlay
```css
background-image:
    radial-gradient(at 80% 20%, rgba(99, 102, 241, 0.15) 0px, transparent 50%),
    radial-gradient(at 20% 80%, rgba(139, 92, 246, 0.1) 0px, transparent 50%),
    radial-gradient(at 50% 50%, rgba(167, 139, 250, 0.05) 0px, transparent 50%);
```

### Footer Üst Çizgi
```css
background: linear-gradient(to right, transparent, rgba(99, 102, 241, 0.5), transparent);
/* Tailwind: bg-gradient-to-r from-transparent via-brand-500/50 to-transparent */
```

---

## 5. TİPOGRAFİ

### Font Ailesi
```
Inter, system-ui, sans-serif
```
**Kaynak:** Google Fonts — `Inter:wght@300;400;500;600;700;800;900`

### Body Varsayılan
```
font-sans antialiased text-gray-800
```

### Boyut & Ağırlık Tablosu

| Kullanım         | Tailwind Class                        | Weight | Size (px)   |
|------------------|---------------------------------------|--------|-------------|
| Hero H1          | `text-5xl lg:text-7xl font-extrabold` | 800    | 48 / 72     |
| Section H2       | `text-4xl lg:text-5xl font-bold`      | 700    | 36 / 48     |
| Card H3          | `text-xl font-bold`                   | 700    | 20          |
| Navbar Link      | `text-sm font-medium`                 | 500    | 14          |
| Buton Text       | `text-sm font-semibold`               | 600    | 14          |
| Body Paragraph   | `text-lg text-gray-600`               | 400    | 18          |
| Small/Footer     | `text-sm text-gray-300`               | 400    | 14          |
| Logo             | `text-xl font-bold text-gray-900`     | 700    | 20          |

---

## 6. BUTON STİLLERİ

### Primary (Filled) — "Ücretsiz Dene"
```html
<a class="px-8 py-4 rounded-2xl text-lg font-semibold text-white
          bg-gradient-to-r from-brand-500 to-purple-600
          shadow-xl shadow-brand-500/30
          hover:shadow-brand-500/50 hover:scale-105
          transition-all duration-300">
    Ücretsiz Dene
</a>
```
- Border Radius: `rounded-2xl` (16px)
- Padding: 32px / 16px
- Shadow: `0 25px 50px -12px rgba(99, 102, 241, 0.3)`

### Secondary (Outlined) — "Giriş Yap" (Açık BG)
```html
<a class="px-5 py-2.5 rounded-xl text-sm font-semibold
          text-brand-700 border-2 border-brand-200
          hover:border-brand-500 hover:bg-brand-50
          transition-all duration-300">
    <i class="fas fa-sign-in-alt mr-1.5"></i> Giriş Yap
</a>
```

### Ghost (Dark BG) — "Giriş Yap" (Hero üzerinde)
```html
<a class="px-8 py-4 rounded-2xl text-lg font-semibold text-white
          bg-white/20 border-2 border-white/50
          hover:bg-white/30
          transition-all duration-300">
    <i class="fas fa-sign-in-alt mr-2"></i> Giriş Yap
</a>
```

### Navbar CTA (Küçük)
```html
<a class="px-6 py-2.5 rounded-xl text-sm font-semibold text-white
          bg-gradient-to-r from-brand-500 to-purple-600
          shadow-lg shadow-brand-500/30
          hover:shadow-brand-500/50 hover:scale-105
          transition-all duration-300">
    Ücretsiz Dene
</a>
```

---

## 7. KART STİLLERİ

### Feature Card (Açık arka plan)
```html
<div class="bg-white rounded-3xl p-8
            border border-gray-100
            shadow-lg shadow-gray-100/50
            hover:shadow-xl hover:shadow-brand-100/50
            hover:border-brand-100
            transition-all duration-500">
```

### Module Card (Açık arka plan)
```html
<div class="bg-white rounded-2xl p-6
            border border-gray-100
            shadow-lg shadow-gray-100/50
            hover:shadow-xl hover:border-brand-200
            transition-all duration-500">
```

### Pricing Card (Normal)
```html
<div class="bg-white rounded-3xl p-8
            border border-gray-200
            shadow-xl">
```

### Pricing Card (Featured / Popüler)
```html
<div class="bg-gradient-to-br from-brand-500 to-purple-600
            rounded-3xl p-8
            shadow-2xl text-white
            ring-4 ring-brand-200
            scale-105">
```

### Glass Card (Koyu BG üzerinde)
```css
.glass {
    background: rgba(255, 255, 255, 0.1);
    backdrop-filter: blur(20px);
    border: 1px solid rgba(255, 255, 255, 0.2);
}
```

### Glass White Card (Açık BG, bulanık)
```css
.glass-white {
    background: rgba(255, 255, 255, 0.7);
    backdrop-filter: blur(20px);
    border: 1px solid rgba(255, 255, 255, 0.3);
}
```

---

## 8. İKON SİSTEMİ

| Kütüphane      | Versiyon | CDN                                                              |
|----------------|----------|------------------------------------------------------------------|
| Font Awesome   | 6.5.1    | `cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/css/all.min.css` |

### İkon Kutu Stili (Feature/Module)
```html
<div class="w-14 h-14 rounded-2xl
            bg-gradient-to-br from-brand-500/10 to-purple-500/10
            flex items-center justify-center
            text-brand-600 text-2xl
            transition-all duration-300
            group-hover:from-brand-500 group-hover:to-purple-600
            group-hover:text-white group-hover:scale-110 group-hover:rotate-[-5deg]">
    <i class="fas fa-chart-line"></i>
</div>
```

---

## 9. LOGO

### Logo Kutusu
```html
<div class="w-10 h-10 rounded-xl
            bg-gradient-to-br from-brand-500 to-purple-600
            flex items-center justify-center
            shadow-lg shadow-brand-500/30">
    <span class="text-white font-bold text-lg">EF</span>
</div>
```

### Logo Text
```html
<span class="text-xl font-bold text-gray-900">
    Emare <span class="gradient-text">Finance</span>
</span>
```
- "EF" → beyaz (`text-white font-bold text-lg`) kutu içinde
- "Emare" → `text-gray-900 font-bold text-xl`
- "Finance" → gradient-text (`135deg, #4f46e5, #7c3aed, #6d28d9`)

---

## 10. GÖLGE (Shadow) SİSTEMİ

| Kullanım          | Değer                                            |
|-------------------|--------------------------------------------------|
| Navbar            | `shadow-sm`                                      |
| Küçük Buton       | `shadow-lg shadow-brand-500/30`                  |
| Büyük Buton       | `shadow-xl shadow-brand-500/30`                  |
| Feature Card      | `shadow-lg shadow-gray-100/50`                   |
| Card Hover        | `0 25px 50px -12px rgba(99, 102, 241, 0.15)`    |
| Pricing Featured  | `shadow-2xl`                                     |
| Glow Efekti       | `0 0 40px rgba(99, 102, 241, 0.3)`              |
| Logo              | `shadow-lg shadow-brand-500/30`                  |

---

## 11. BORDER RADIUS

| Eleman         | Tailwind       | Pixel  |
|----------------|----------------|--------|
| Küçük Buton    | `rounded-xl`   | 12px   |
| Büyük Buton    | `rounded-2xl`  | 16px   |
| Feature Card   | `rounded-3xl`  | 24px   |
| Module Card    | `rounded-2xl`  | 16px   |
| İkon Kutusu    | `rounded-2xl`  | 16px   |
| Logo Kutusu    | `rounded-xl`   | 12px   |
| Input/Form     | `rounded-xl`   | 12px   |
| Badge          | `rounded-full` | 9999px |

---

## 12. ANİMASYONLAR

### Float (Blob / Dekoratif)
```css
@keyframes float {
    0%, 100% { transform: translateY(0px); }
    50% { transform: translateY(-20px); }
}
/* Kullanım: animate-float (6s), animate-float-delayed (6s, 2s delay), animate-float-slow (8s, 1s delay) */
```

### Gradient BG Animasyonu
```css
@keyframes gradient {
    0%, 100% { background-position: 0% 50%; }
    50% { background-position: 100% 50%; }
}
/* Kullanım: animate-gradient (8s) */
```

### Fade Up (Scroll Reveal)
```css
@keyframes fadeUp {
    0% { opacity: 0; transform: translateY(30px); }
    100% { opacity: 1; transform: translateY(0); }
}
/* Kullanım: animate-fade-up (0.6s) */
```

### Slide Right
```css
@keyframes slideRight {
    0% { opacity: 0; transform: translateX(-30px); }
    100% { opacity: 1; transform: translateX(0); }
}
/* Kullanım: animate-slide-right (0.6s) */
```

### Pulse Soft
```css
@keyframes pulseSoft {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.7; }
}
/* Kullanım: animate-pulse-soft (3s) */
```

### Blob Morph
```css
@keyframes morph {
    0%, 100% { border-radius: 42% 58% 70% 30% / 45% 45% 55% 55%; }
    33% { border-radius: 70% 30% 46% 54% / 30% 29% 71% 70%; }
    66% { border-radius: 30% 70% 70% 30% / 58% 42% 58% 42%; }
}
```

### Shine Efekti
```css
@keyframes shine {
    0%, 100% { transform: translateX(-50%) translateY(-50%) rotate(30deg); }
    50% { transform: translateX(50%) translateY(50%) rotate(30deg); }
}
```

### Scroll Reveal (JavaScript)
```css
.scroll-reveal {
    opacity: 0;
    transform: translateY(40px);
    transition: all 0.8s cubic-bezier(0.4, 0, 0.2, 1);
}
.scroll-reveal.revealed {
    opacity: 1;
    transform: translateY(0);
}
```

---

## 13. SAYFA YAPISI (Layout)

| Öğe               | Değer                                              |
|--------------------|----------------------------------------------------|
| Max Width          | `max-w-7xl` (1280px)                               |
| Container Padding  | `px-4 sm:px-6 lg:px-8`                             |
| Navbar Height      | `h-20` (80px)                                      |
| Section Padding    | `py-24 lg:py-32`                                   |
| Grid (Feature)     | `grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-8`  |
| Grid (Module)      | `grid-cols-2 sm:grid-cols-3 md:grid-cols-4 lg:grid-cols-5 gap-6` |
| Grid (Footer)      | `grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-12` |

---

## 14. NAVBAR

### Yapı
```html
<nav class="fixed top-0 left-0 right-0 z-50
            bg-white/95 backdrop-blur-xl shadow-sm
            transition-all duration-500">
    <!-- h-20, max-w-7xl -->
</nav>
```

### Navbar Linkleri
```
text-sm font-medium text-gray-600
hover:text-brand-600 hover:bg-brand-500/10
px-4 py-2 rounded-lg
```

### Mobile Menu
```
bg-white/95 backdrop-blur-xl
border-t border-gray-100 shadow-xl
```

---

## 15. FOOTER

```html
<footer class="bg-gray-950 text-gray-300">
    <!-- Top border: h-px bg-gradient-to-r from-transparent via-brand-500/50 to-transparent -->
    <!-- Content: max-w-7xl, pt-20 pb-12 -->
    <!-- Bottom bar: border-t border-white/5, text-gray-400 -->
</footer>
```

### Footer Sosyal İkon
```
w-10 h-10 rounded-lg
bg-white/10 hover:bg-brand-500/20
text-gray-300 hover:text-brand-400
```

---

## 16. FORM ELEMANLARI

### Text Input
```html
<input class="w-full px-4 py-3 rounded-xl
              border border-gray-200
              focus:border-brand-500 focus:ring-2 focus:ring-brand-500/20
              outline-none transition-all duration-300
              text-gray-800 placeholder-gray-400">
```

### Textarea
```html
<textarea class="w-full px-4 py-3 rounded-xl
                 border border-gray-200
                 focus:border-brand-500 focus:ring-2 focus:ring-brand-500/20
                 outline-none transition-all duration-300
                 text-gray-800 placeholder-gray-400"
          rows="4"></textarea>
```

### Form Submit Button
```html
<button class="w-full px-8 py-4 rounded-xl
               bg-gradient-to-r from-brand-500 to-purple-600
               text-white font-semibold
               shadow-lg shadow-brand-500/30
               hover:shadow-brand-500/50 hover:scale-[1.02]
               transition-all duration-300">
```

---

## 17. SCROLLBAR

```css
::-webkit-scrollbar { width: 8px; }
::-webkit-scrollbar-track { background: #1e1b4b; }
::-webkit-scrollbar-thumb { background: #4f46e5; border-radius: 4px; }
::-webkit-scrollbar-thumb:hover { background: #6366f1; }
```

---

## 18. BAĞIMLILIKLAR (Dependencies)

| Kaynak            | Versiyon / URL                                             |
|-------------------|------------------------------------------------------------|
| Tailwind CSS      | CDN — `cdn.tailwindcss.com`                                |
| Alpine.js         | 3.x — `cdn.jsdelivr.net/npm/alpinejs@3.x.x/dist/cdn.min.js` |
| Alpine Collapse   | 3.x — `cdn.jsdelivr.net/npm/@alpinejs/collapse@3.x.x`     |
| Alpine Intersect  | 3.x — `cdn.jsdelivr.net/npm/@alpinejs/intersect@3.x.x`    |
| Font Awesome      | 6.5.1 — `cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/css/all.min.css` |
| Inter Font        | Google Fonts — `fonts.googleapis.com/css2?family=Inter`    |
| Chart.js          | 4.x (panel/dashboard içi)                                 |

---

## 19. TAILWIND CONFIG

```javascript
tailwind.config = {
    theme: {
        extend: {
            fontFamily: {
                sans: ['Inter', 'system-ui', 'sans-serif'],
            },
            colors: {
                brand: {
                    50:  '#eef2ff',
                    100: '#e0e7ff',
                    200: '#c7d2fe',
                    300: '#a5b4fc',
                    400: '#818cf8',
                    500: '#6366f1',
                    600: '#4f46e5',
                    700: '#4338ca',
                    800: '#3730a3',
                    900: '#312e81',
                    950: '#1e1b4b',
                }
            },
            animation: {
                'float':         'float 6s ease-in-out infinite',
                'float-delayed': 'float 6s ease-in-out 2s infinite',
                'float-slow':    'float 8s ease-in-out 1s infinite',
                'gradient':      'gradient 8s ease infinite',
                'fade-up':       'fadeUp 0.6s ease-out forwards',
                'slide-right':   'slideRight 0.6s ease-out forwards',
                'counter':       'counter 2s ease-out forwards',
                'pulse-soft':    'pulseSoft 3s ease-in-out infinite',
            },
            keyframes: {
                float: {
                    '0%, 100%': { transform: 'translateY(0px)' },
                    '50%':      { transform: 'translateY(-20px)' },
                },
                gradient: {
                    '0%, 100%': { backgroundPosition: '0% 50%' },
                    '50%':      { backgroundPosition: '100% 50%' },
                },
                fadeUp: {
                    '0%':   { opacity: '0', transform: 'translateY(30px)' },
                    '100%': { opacity: '1', transform: 'translateY(0)' },
                },
                slideRight: {
                    '0%':   { opacity: '0', transform: 'translateX(-30px)' },
                    '100%': { opacity: '1', transform: 'translateX(0)' },
                },
                pulseSoft: {
                    '0%, 100%': { opacity: '1' },
                    '50%':      { opacity: '0.7' },
                }
            }
        }
    }
}
```

---

*Bu dosya POS yazılımı ve tüm Emare Finance ürünlerinde tutarlı tasarım için referans olarak kullanılmalıdır.*
