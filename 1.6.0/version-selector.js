document.addEventListener("DOMContentLoaded", async () => {
  const path = window.location.pathname.split("/").filter(Boolean);
  if (path.length < 2 || path[0] !== "STM32Ethernet") {
    return;
  }

  const siteRoot = `${window.location.origin}/STM32Ethernet/`;
  let versions;
  try {
    const response = await fetch(`${siteRoot}versions.json`);
    if (!response.ok) {
      return;
    }
    versions = await response.json();
  } catch {
    return;
  }

  const selector = document.getElementById("doc-version");
  const selectorContainer = document.getElementById("doc-version-selector");
  if (!selector || !selectorContainer) {
    return;
  }

  const currentVersion = path[1];
  for (const entry of versions) {
    for (const name of [entry.version, ...entry.aliases]) {
      const option = document.createElement("option");
      option.value = name;
      option.textContent = name === "latest" ? `latest (${entry.version})` : name;
      option.selected = name === currentVersion;
      selector.add(option);
    }
  }

  selector.addEventListener("change", () => {
    const pagePath = path.slice(2).map(encodeURIComponent).join("/");
    const target = `${siteRoot}${encodeURIComponent(selector.value)}/${pagePath}`;
    window.location.assign(`${target}${window.location.search}${window.location.hash}`);
  });
  selectorContainer.hidden = false;
});
