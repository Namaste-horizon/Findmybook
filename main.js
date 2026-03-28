// Get username from URL
const params = new URLSearchParams(window.location.search);
const currentUser = params.get("user");
if(!currentUser){
    alert("Please login first!");
    window.location.href = "auth.html";
}

document.getElementById('userName').innerText = currentUser;

// Dashboard
let bookings = JSON.parse(localStorage.getItem("bookings")) || {};
if(!bookings[currentUser]) bookings[currentUser] = [];
updateDashboard();

function updateDashboard(){
    const div = document.getElementById('bookingList');
    div.innerHTML = "";
    if(bookings[currentUser].length === 0){
        div.innerHTML = "<p>No bookings yet</p>";
        return;
    }
    bookings[currentUser].forEach(b=>{
        div.innerHTML += `<div class="card">${b}</div>`;
    });
}

function logout(){
    window.location.href = "auth.html";
}

// Search
const demoBooks = [
    {title:"Data Structures", author:"Mark Allen"},
    {title:"Algorithms", author:"CLRS"},
    {title:"Artificial Intelligence", author:"Russell"}
];

function searchBooks(){
    const container = document.getElementById('results');
    container.innerHTML = '';
    demoBooks.forEach(book=>{
        const div = document.createElement('div');
        div.className='result-card';
        div.innerHTML = `<h3>${book.title}</h3>
                         <p>${book.author || ""}</p>
                         <button onclick="rentBook('${book.title}')">Rent</button>`;
        container.appendChild(div);
    });
}

// Booking
function bookSeat(){
    const lib = document.getElementById('libraryInput').value;
    const date = document.getElementById('dateInput').value;
    const time = document.getElementById('timeInput').value;

    bookings[currentUser].push(`Library:${lib} | ${date} | ${time}`);
    localStorage.setItem("bookings", JSON.stringify(bookings));
    updateDashboard();
    alert("Seat booked!");
}

// Rent book
function rentBook(book){
    bookings[currentUser].push(book + " rented");
    localStorage.setItem("bookings", JSON.stringify(bookings));
    updateDashboard();
}

// Scroll
function scrollToSection(id){
    document.getElementById(id).scrollIntoView({behavior:'smooth'});
}