// ── Brick Docs: Interactive Features ──

document.addEventListener('DOMContentLoaded', () => {
    // ── Navbar scroll shadow ──
    const navbar = document.querySelector('.navbar');
    let lastScroll = 0;

    window.addEventListener('scroll', () => {
        const scrollY = window.scrollY;
        if (scrollY > 60) {
            navbar.classList.add('scrolled');
        } else {
            navbar.classList.remove('scrolled');
        }
        lastScroll = scrollY;
    }, { passive: true });

    // ── Smooth reveal for feature cards ──
    const observerOptions = {
        root: null,
        rootMargin: '0px 0px -50px 0px',
        threshold: 0.1
    };

    const observer = new IntersectionObserver((entries) => {
        entries.forEach((entry, i) => {
            if (entry.isIntersecting) {
                // Stagger the animation
                const delay = Array.from(entry.target.parentNode.children).indexOf(entry.target) * 80;
                entry.target.style.transitionDelay = `${delay}ms`;
                entry.target.classList.add('visible');
                observer.unobserve(entry.target);
            }
        });
    }, observerOptions);

    document.querySelectorAll('.feature-card, .showcase-item').forEach(el => {
        el.style.opacity = '0';
        el.style.transform = 'translateY(30px)';
        el.style.transition = 'opacity 0.6s ease-out, transform 0.6s ease-out';
        observer.observe(el);
    });

    // ── Apply visible class ──
    document.addEventListener('visibility-changed', () => {
        document.querySelectorAll('.feature-card.visible, .showcase-item.visible').forEach(el => {
            el.style.opacity = '1';
            el.style.transform = 'translateY(0)';
        });
    });

    // ── Additional observer for visibility ──
    const visibilityObserver = new MutationObserver(() => {
        document.querySelectorAll('.feature-card.visible, .showcase-item.visible').forEach(el => {
            el.style.opacity = '1';
            el.style.transform = 'translateY(0)';
        });
    });

    visibilityObserver.observe(document.body, { childList: true, subtree: true });

    // ── Syntax highlight hero code ──
    const heroCode = document.querySelector('.hero-code code');
    if (heroCode) {
        const code = heroCode.innerHTML;
        // Simple syntax highlighting is done server-side via span classes
        // This just ensures everything renders properly
    }

    // ── Copy code button ──
    document.querySelectorAll('.hero-code pre, .showcase-item pre').forEach(pre => {
        const copyBtn = document.createElement('button');
        copyBtn.className = 'copy-btn';
        copyBtn.textContent = '📋';
        copyBtn.title = 'Copy code';
        copyBtn.style.cssText = `
            position: absolute;
            top: 8px;
            right: 8px;
            background: var(--bg-secondary);
            border: 1px solid var(--border);
            border-radius: 6px;
            padding: 6px 10px;
            cursor: pointer;
            font-size: 0.9rem;
            opacity: 0;
            transition: opacity 0.2s;
            z-index: 2;
        `;
        pre.style.position = 'relative';
        pre.appendChild(copyBtn);

        pre.addEventListener('mouseenter', () => { copyBtn.style.opacity = '1'; });
        pre.addEventListener('mouseleave', () => { copyBtn.style.opacity = '0'; });

        copyBtn.addEventListener('click', async () => {
            const code = pre.querySelector('code')?.textContent || pre.textContent;
            try {
                await navigator.clipboard.writeText(code);
                copyBtn.textContent = '✅';
                setTimeout(() => { copyBtn.textContent = '📋'; }, 2000);
            } catch {
                copyBtn.textContent = '❌';
                setTimeout(() => { copyBtn.textContent = '📋'; }, 2000);
            }
        });
    });

    // ── Keyboard navigation ──
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') {
            // Close any open menus
        }
    });

    // ── Mobile menu toggle (if needed) ──
    // The nav is simple enough that it doesn't need a hamburger for now

    console.log(`Brick Docs loaded at ${new Date().toLocaleTimeString()}`);
    console.log('Need help? Check the Language Reference!');
});
