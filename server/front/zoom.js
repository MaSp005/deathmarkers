const zoomWrapper = document.createElement("dialog");
let debounce = false;
zoomWrapper.className = "zoom";
document.body.appendChild(zoomWrapper);
zoomWrapper.addEventListener("click", e => {
  if (e.target === zoomWrapper) zoomWrapper.close();
});

[...document.querySelectorAll("p img")].forEach(img => {
  img.setAttribute("tabindex", "0");
  function zoom() {
    let clone = img.cloneNode();
    zoomWrapper.replaceChildren(clone);
    zoomWrapper.append(img.alt);
    zoomWrapper.showModal();
  }
  img.addEventListener("click", zoom);
  img.addEventListener("keypress", e => {
    if (e.key == "Enter") zoom();
    debounce = true;
  });
});

document.addEventListener("keypress", e => {
  if (debounce) return debounce = false;
  if (["Enter", "Escape"].includes(e.key)) {
    if (zoomWrapper.open) e.preventDefault();
    zoomWrapper.close();
  }
});
