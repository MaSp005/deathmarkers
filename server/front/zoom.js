const zoomWrapper = document.createElement("dialog");
zoomWrapper.className = "zoom";
document.body.appendChild(zoomWrapper);
zoomWrapper.addEventListener("click", e => {
  if (e.target === zoomWrapper) zoomWrapper.close();
});

[...document.querySelectorAll("p img")].forEach(img => {
  img.addEventListener("click", () => {
    let clone = img.cloneNode();
    zoomWrapper.replaceChildren(clone);
    zoomWrapper.append(img.alt);
    zoomWrapper.showModal();
  })
});
