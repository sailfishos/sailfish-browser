var listenerRegistered = false;

function updateState() {
    var fullscreenState = document.getElementById("fullscreenState")
    fullscreenState.innerHTML = (document.mozFullScreen)? "Exit fullscreen" : "Go to fullscreen";
}

function toggleFullScreen() {
    if (!listenerRegistered) {
        document.onmozfullscreenchange = updateState;
        listenerRegistered = true;
    }

    if (!document.mozFullScreenElement) {
        document.documentElement.mozRequestFullScreen();
    } else {
        if (document.mozFullScreen) {
            document.mozCancelFullScreen();
        }
    }
}
