function initMap(){
    const map = L.map('mapContainer').setView([30.3165,78.0322],13);
    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);

    if(navigator.geolocation){
        navigator.geolocation.getCurrentPosition(pos=>{
            const lat = pos.coords.latitude;
            const lng = pos.coords.longitude;
            L.marker([lat,lng]).addTo(map).bindPopup("You are here").openPopup();
            map.setView([lat,lng],13);
        });
    }

    const libraries = [
        {name:"Central Library", lat:30.3165, lng:78.0322},
        {name:"City Library", lat:30.31, lng:78.02}
    ];

    libraries.forEach(lib=>{
        L.marker([lib.lat, lib.lng]).addTo(map).bindPopup(lib.name);
    });
}

window.addEventListener("DOMContentLoaded", initMap);