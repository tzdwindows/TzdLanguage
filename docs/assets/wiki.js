/**
 * TzdLang Official Wiki — "From Beginner to Master" Interactive Engine
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
  });

  // 2. Code Block Snippet Copy
  document.querySelectorAll(".btn-copy-snippet").forEach((btn) => {
    btn.addEventListener("click", () => {
      const codePre = btn.closest(".code-block-wrapper").querySelector("pre");
      if (codePre) {
        navigator.clipboard.writeText(codePre.innerText).then(() => {
          const originalText = btn.innerHTML;
          btn.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="#34D399" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"></polyline></svg><span style="color:#34D399;">已复制</span>`;
          setTimeout(() => {
            btn.innerHTML = originalText;
          }, 2000);
        });
      }
    });
  });

  // 3. Search Filter in Sidebar
  const searchInput = document.getElementById("wiki-search-input");
  if (searchInput) {
    searchInput.addEventListener("input", (e) => {
      const query = e.target.value.toLowerCase().trim();
      const links = document.querySelectorAll(".sidebar-link");
      links.forEach((link) => {
        const text = link.innerText.toLowerCase();
        if (text.includes(query)) {
          link.style.display = "flex";
        } else {
          link.style.display = "none";
        }
      });
    });

    // Keyboard shortcut (Cmd+K / Ctrl+K)
    window.addEventListener("keydown", (e) => {
      if ((e.metaKey || e.ctrlKey) && e.key === "k") {
        e.preventDefault();
        searchInput.focus();
      }
    });
  }

  // 4. ScrollSpy for TOC and Sidebar Navigation
  const articles = document.querySelectorAll(".wiki-article");
  const sidebarLinks = document.querySelectorAll(".sidebar-link");
  const tocLinks = document.querySelectorAll(".toc-item a");

  function updateActiveNav() {
    let currentId = "";
    const scrollPos = window.scrollY + 120;

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
      sidebarLinks.forEach((link) => {
        if (link.getAttribute("href") === `#${currentId}`) {
          link.classList.add("active");
        } else {
          link.classList.remove("active");
        }
      });

      tocLinks.forEach((tLink) => {
        const parentLi = tLink.parentElement;
        if (tLink.getAttribute("href") === `#${currentId}`) {
          parentLi.classList.add("active");
        } else {
          parentLi.classList.remove("active");
        }
      });
    }
  }

  window.addEventListener("scroll", updateActiveNav);
  updateActiveNav();
});
