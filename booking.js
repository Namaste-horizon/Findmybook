let bookings = JSON.parse(localStorage.getItem("bookings")) || {};

function bookSeat(){
    if(!currentUser){
        alert("Login first");
        return;
    }

    const lib = libraryInput.value;
    const date = dateInput.value;
    const time = timeInput.value;

    if(!bookings[currentUser]){
        bookings[currentUser]=[];
    }

    bookings[currentUser].push(`Library:${lib} | ${date} | ${time}`);

    localStorage.setItem("bookings", JSON.stringify(bookings));

    updateDashboard();
}

function rentBook(book){
    if(!currentUser){
        alert("Login first");
        return;
    }

    bookings[currentUser].push(book + " rented");
    localStorage.setItem("bookings", JSON.stringify(bookings));

    updateDashboard();
}