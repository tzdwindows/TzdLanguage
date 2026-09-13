/**
 * TzdLang Official Wiki — "From Beginner to Master"
 * Apple Developer HIG Fluid Physics Motion & Interactive Engine
 */

document.addEventListener("DOMContentLoaded", () => {
  // 1. Reading Progress Bar
  const progressBar = document.getElementById("reading-progress-bar");
  window.addEventListener("scroll", () => {
    const winScroll = document.documentElement.scrollTop || document.body.scrollTop;
    const height = document.documentElement.scrollHeight - document.documentElement.clientHeight;
    const scrolled = height > 0 ? (winScroll / height) * 100 : 0;
    if (progressBar) {
      progressBar.style.width = scrolled + "%";
    }
  }, { passive: true });

  // 2. Code Block Snippet Copy with Apple Haptic-like Feedback
  document.querySelectorAll(".btn-copy-snippet").forEach((btn) => {
    btn.addEventListener("click", () => {
      const codePre = btn.closest(".code-block-wrapper").querySelector("pre code");
      if (codePre) {
        navigator.clipboard.writeText(codePre.innerText).then(() => {
          const originalHTML = btn.innerHTML;
          btn.classList.add("copied");
          btn.innerHTML = `
            <svg xmlns="http://www.w3.org/2000/svg" width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="#34D399" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
              <polyline points="20 6 9 17 4 12"></polyline>
            </svg>
            <span style="color:#34D399; font-weight:500;">已复制</span>
          `;

          setTimeout(() => {
            btn.classList.remove("copied");
            btn.innerHTML = originalHTML;
          }, 2000);
        }).catch(err => {
          console.error("Clipboard copy failed:", err);
        });
      }
    });
  });

  // 3. Apple Spotlight Style Search Filter in Sidebar
  const searchInput = document.getElementById("wiki-search-input");
  if (searchInput) {
    searchInput.addEventListener("input", (e) => {
      const query = e.target.value.toLowerCase().trim();
      const links = document.querySelectorAll(".sidebar-link");
      const groups = document.querySelectorAll(".sidebar-group-title");

      links.forEach((link) => {
        const text = link.innerText.toLowerCase();
        if (text.includes(query)) {
          link.style.display = "flex";
        } else {
          link.style.display = "none";
        }
      });

      // Hide group title if all its siblings are hidden
      groups.forEach((group) => {
        const list = group.nextElementSibling;
        if (list && list.classList.contains("sidebar-nav-list")) {
          const visibleLinks = list.querySelectorAll(".sidebar-link[style*='display: flex']");
          const anyVisible = query === "" || visibleLinks.length > 0;
          group.style.display = anyVisible ? "block" : "none";
        }
      });
    });

    // Keyboard shortcut (Cmd+K / Ctrl+K)
    window.addEventListener("keydown", (e) => {
      if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === "k") {
        e.preventDefault();
        searchInput.focus();
        searchInput.select();
      } else if (e.key === "Escape" && document.activeElement === searchInput) {
        searchInput.blur();
      }
    });
  }

  // 4. Mobile Sidebar Drawer Toggle
  const mobileToggleBtn = document.getElementById("btn-mobile-toggle");
  const sidebar = document.getElementById("wiki-sidebar");
  if (mobileToggleBtn && sidebar) {
    mobileToggleBtn.addEventListener("click", () => {
      sidebar.classList.toggle("open");
    });

    // Close on clicking outside
    document.addEventListener("click", (e) => {
      if (window.innerWidth <= 860 && sidebar.classList.contains("open")) {
        if (!sidebar.contains(e.target) && !mobileToggleBtn.contains(e.target)) {
          sidebar.classList.remove("open");
        }
      }
    });
  }

  // 5. ScrollSpy & Fluid TOC Sliding Indicator
  const articles = document.querySelectorAll(".wiki-article");
  const sidebarLinks = document.querySelectorAll(".sidebar-link");
  const tocLinks = document.querySelectorAll(".toc-item a");
  const tocPill = document.getElementById("toc-active-pill");

  function updateTOCPill(activeItem) {
    if (!tocPill || !activeItem) {
      if (tocPill) tocPill.style.opacity = "0";
      return;
    }
    const navContainer = activeItem.closest(".toc-nav-container");
    if (!navContainer) return;

    const offsetTop = activeItem.offsetTop;
    const itemHeight = activeItem.offsetHeight;

    tocPill.style.opacity = "1";
    tocPill.style.transform = `translateY(${offsetTop}px)`;
    tocPill.style.height = `${itemHeight}px`;
  }

  function updateActiveNav() {
    let currentId = "";
    const scrollPos = window.scrollY + 130;

    articles.forEach((art) => {
      const top = art.offsetTop;
      const height = art.offsetHeight;
      if (scrollPos >= top && scrollPos < top + height) {
        currentId = art.getAttribute("id");
      }
    });

    if (!currentId && articles.length > 0 && window.scrollY < articles[0].offsetTop) {
      currentId = articles[0].getAttribute("id");
    }

    if (currentId) {
      // Update Sidebar Links
      sidebarLinks.forEach((link) => {
        if (link.getAttribute("href") === `#${currentId}`) {
          link.classList.add("active");
        } else {
          link.classList.remove("active");
        }
      });

      // Update Right TOC
      let activeTocItem = null;
      tocLinks.forEach((tLink) => {
        const parentLi = tLink.parentElement;
        if (tLink.getAttribute("href") === `#${currentId}`) {
          parentLi.classList.add("active");
          activeTocItem = parentLi;
        } else {
          parentLi.classList.remove("active");
        }
      });

      if (activeTocItem) {
        updateTOCPill(activeTocItem);
      }
    }
  }

  let ticking = false;
  window.addEventListener("scroll", () => {
    if (!ticking) {
      window.requestAnimationFrame(() => {
        updateActiveNav();
        ticking = false;
      });
      ticking = true;
    }
  }, { passive: true });

  // Initial call
  updateActiveNav();

  // Smooth scroll with subtle spotlight on target article
  document.querySelectorAll('a[href^="#"]').forEach((anchor) => {
    anchor.addEventListener("click", function(e) {
      const targetId = this.getAttribute("href");
      if (targetId.startsWith("#ch")) {
        const targetElement = document.querySelector(targetId);
        if (targetElement) {
          e.preventDefault();
          targetElement.scrollIntoView({ behavior: "smooth" });
          history.pushState(null, "", targetId);

          // Close mobile sidebar if open
          if (sidebar && sidebar.classList.contains("open")) {
            sidebar.classList.remove("open");
          }
        }
      }
    });
  });
});
