document.addEventListener("DOMContentLoaded", function () {
    const bannerTrack = document.getElementById("bannerTrack");

    if (!bannerTrack) return;

    bannerTrack.innerHTML += bannerTrack.innerHTML;

    let position = 0;
    const speed = 1;

    function animate() {
        position -= speed;

        if (Math.abs(position) >= bannerTrack.scrollWidth / 2) {
            position = 0;
        }

        bannerTrack.style.transform = `translateX(${position}px)`;
        requestAnimationFrame(animate);
    }

    animate();
});