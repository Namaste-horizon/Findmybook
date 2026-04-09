const API_BASE = "http://127.0.0.1:8000";
const SESSION_KEY = "findmybookSession";

const state = {
    session: null,
    role: "user",
    managedLibraryId: "",
    libraries: [],
    nearbyLibraries: [],
    userBooks: [],
    userBookings: [],
    libraryBooks: [],
    libraryBookings: [],
    adminBooks: [],
    adminBookings: [],
    accounts: {
        admins: [],
        users: [],
        libraries: [],
    },
    currentQuery: "",
    editingBookId: "",
    editingLibraryId: "",
    map: null,
    markersLayer: null,
    userMarker: null,
    location: null,
};

function setStatus(message, isError = false) {
    const status = document.getElementById("appStatus");
    if (!status) {
        return;
    }

    status.textContent = message;
    status.className = `status-banner ${isError ? "error" : "success"}`.trim();
}

async function requestJson(path, options = {}) {
    const response = await fetch(`${API_BASE}${path}`, {
        headers: {
            "Content-Type": "application/json",
            ...(options.headers || {}),
        },
        ...options,
    });

    const payload = await response.json().catch(() => ({}));
    if (!response.ok) {
        throw new Error(payload.error || "Request failed");
    }

    return payload;
}

function getSession() {
    try {
        return JSON.parse(localStorage.getItem(SESSION_KEY) || "null");
    } catch (error) {
        return null;
    }
}

function deriveLibraryIdFromCode(code = "") {
    if (!/^l\d+$/.test(code)) {
        return "";
    }

    return `lib-${code.slice(1)}`;
}

function ensureSession() {
    state.session = getSession();
    if (!state.session?.email) {
        window.location.href = "auth.html";
        return false;
    }

    state.role = state.session.role || "user";
    state.managedLibraryId = state.session.library_id || deriveLibraryIdFromCode(state.session.code || "");
    return true;
}

function formatName(person = state.session) {
    const firstName = person?.fname || "";
    const lastName = person?.lname || "";
    return `${firstName} ${lastName}`.trim() || person?.email || "FindMyBook User";
}

function roleLabel(role) {
    if (role === "admin") {
        return "Admin";
    }
    if (role === "lib") {
        return "Library Manager";
    }
    return "User";
}

function scrollToSection(id) {
    document.getElementById(id)?.scrollIntoView({ behavior: "smooth" });
}

function logout() {
    localStorage.removeItem(SESSION_KEY);
    window.location.href = "auth.html";
}

function getManagedLibrary() {
    return state.libraries.find((library) => library.id === state.managedLibraryId) || null;
}

function activeRentalCount(bookings) {
    return bookings.filter((booking) =>
        booking.category === "rental" &&
        booking.status !== "returned" &&
        booking.status !== "cancelled"
    ).length;
}

function hasActiveRental(bookId) {
    return state.userBookings.some((booking) =>
        booking.category === "rental" &&
        booking.book_id === bookId &&
        booking.status !== "returned" &&
        booking.status !== "cancelled"
    );
}

function showRolePanels() {
    document.querySelectorAll("[data-roles]").forEach((element) => {
        const allowedRoles = (element.dataset.roles || "").split(/\s+/).filter(Boolean);
        element.hidden = !allowedRoles.includes(state.role);
    });
}

function populateHeader() {
    const title = document.getElementById("welcomeTitle");
    const subtitle = document.getElementById("welcomeSubtitle");
    const sessionMeta = document.getElementById("sessionMeta");
    const badge = document.getElementById("roleBadge");

    const managedLibrary = getManagedLibrary();
    badge.textContent = roleLabel(state.role);

    if (state.role === "admin") {
        title.textContent = "Campus-wide library control in one dashboard";
        subtitle.textContent = "Create libraries, inspect accounts, track books, and monitor bookings through the C++ backend.";
    } else if (state.role === "lib") {
        title.textContent = "Manage your library inventory and booking activity";
        subtitle.textContent = "Update books, monitor study seats, and keep one library in sync with the shared system.";
    } else {
        title.textContent = "Search, locate, rent, and reserve";
        subtitle.textContent = "Find books across libraries, see nearby branches, rent titles, and book study seats from one place.";
    }

    const metaParts = [
        `${formatName()} (${state.session.email})`,
        `Code: ${state.session.code || "pending"}`,
        `Role: ${roleLabel(state.role)}`,
        "Backend: C++",
    ];

    if (managedLibrary) {
        metaParts.push(`Library: ${managedLibrary.name}`);
    }

    sessionMeta.textContent = metaParts.join(" | ");
}

function updateOverviewStats() {
    const cards = [
        {
            label: document.getElementById("stat1Label"),
            value: document.getElementById("stat1Value"),
        },
        {
            label: document.getElementById("stat2Label"),
            value: document.getElementById("stat2Value"),
        },
        {
            label: document.getElementById("stat3Label"),
            value: document.getElementById("stat3Value"),
        },
        {
            label: document.getElementById("stat4Label"),
            value: document.getElementById("stat4Value"),
        },
    ];

    let stats;
    if (state.role === "admin") {
        stats = [
            ["Libraries", state.libraries.length],
            ["Books", state.adminBooks.length],
            ["Accounts", state.accounts.admins.length + state.accounts.users.length + state.accounts.libraries.length],
            ["Bookings", state.adminBookings.length],
        ];
    } else if (state.role === "lib") {
        stats = [
            ["My Books", state.libraryBooks.length],
            ["Available Copies", state.libraryBooks.reduce((sum, book) => sum + (book.available_copies || 0), 0)],
            ["Seat Bookings", state.libraryBookings.filter((booking) => booking.category === "seat").length],
            ["Rentals", state.libraryBookings.filter((booking) => booking.category === "rental").length],
        ];
    } else {
        stats = [
            ["Books Found", state.userBooks.length],
            ["Nearby Libraries", (state.nearbyLibraries.length || state.libraries.length)],
            ["My Bookings", state.userBookings.length],
            ["Active Rentals", activeRentalCount(state.userBookings)],
        ];
    }

    stats.forEach((item, index) => {
        cards[index].label.textContent = item[0];
        cards[index].value.textContent = String(item[1]);
    });
}

function renderSeatLibraryOptions() {
    const select = document.getElementById("seatLibrary");
    if (!select) {
        return;
    }

    select.innerHTML = '<option value="">Select library</option>';
    state.libraries.forEach((library) => {
        const option = document.createElement("option");
        option.value = library.id;
        option.textContent = `${library.name}${library.city ? `, ${library.city}` : ""}`;
        select.appendChild(option);
    });
}

function renderUserSearchSummary() {
    const summary = document.getElementById("searchSummary");
    if (!summary) {
        return;
    }

    if (state.currentQuery) {
        summary.textContent = `${state.userBooks.length} result(s) for "${state.currentQuery}".`;
    } else {
        summary.textContent = `Showing all ${state.userBooks.length} books connected to the backend.`;
    }
}

function renderUserBooks() {
    const container = document.getElementById("results");
    if (!container) {
        return;
    }

    container.innerHTML = "";
    if (state.userBooks.length === 0) {
        container.innerHTML = '<div class="card"><p>No books matched your search.</p></div>';
        renderUserSearchSummary();
        return;
    }

    state.userBooks.forEach((book) => {
        const alreadyRented = hasActiveRental(book.id);
        const canRent = Boolean(book.available) && !alreadyRented;
        const card = document.createElement("div");
        card.className = "result-card";
        card.innerHTML = `
            <h3>${book.title}</h3>
            <p class="meta">Author: ${book.author || "Unknown"}</p>
            <p class="meta">Genre: ${book.genre || "General"}</p>
            <p class="meta">Library: ${book.library || "Unassigned"}</p>
            <p class="meta">Available copies: ${book.available_copies ?? 0}</p>
            <button class="primary-btn" onclick="rentBook('${book.id}')" ${canRent ? "" : "disabled"}>
                ${alreadyRented ? "Already Rented" : (book.available ? "Rent Book" : "Unavailable")}
            </button>
        `;
        container.appendChild(card);
    });

    renderUserSearchSummary();
}

function bookingDetailLine(booking) {
    if (booking.category === "seat") {
        const seatText = booking.seat_number ? ` | Seat ${booking.seat_number}` : "";
        return `${booking.library || "Library"} | ${booking.date || "Date pending"} | ${booking.time || "Time pending"}${seatText}`;
    }

    return `${booking.title || "Book"}${booking.book_id ? ` | ${booking.book_id}` : ""}${booking.library ? ` | ${booking.library}` : ""}`;
}

function renderUserBookings() {
    const container = document.getElementById("bookingList");
    if (!container) {
        return;
    }

    container.innerHTML = "";
    if (state.userBookings.length === 0) {
        container.innerHTML = '<div class="card"><p>No bookings yet. Rent a book or reserve a study seat to get started.</p></div>';
        return;
    }

    state.userBookings.forEach((booking) => {
        const card = document.createElement("div");
        card.className = "card";
        card.innerHTML = `
            <h3>${booking.category === "seat" ? "Seat Booking" : "Book Rental"}</h3>
            <p>${bookingDetailLine(booking)}</p>
            <span class="status-pill">${booking.status || "active"}</span>
            <div class="actions">
                <button class="secondary-btn" onclick="editUserBooking('${booking.id}')">Edit</button>
                <button class="danger-btn" onclick="deleteUserBooking('${booking.id}')">Delete</button>
            </div>
        `;
        container.appendChild(card);
    });
}

function renderNearbyLibraries() {
    const container = document.getElementById("nearbyLibraryGrid");
    if (!container) {
        return;
    }

    const libraries = state.nearbyLibraries.length ? state.nearbyLibraries : state.libraries;
    container.innerHTML = "";

    libraries.forEach((library) => {
        const card = document.createElement("div");
        card.className = "library-card";
        const distanceText = library.distance_km !== undefined ? `${library.distance_km} km away` : "Distance available after location access";
        card.innerHTML = `
            <h3>${library.name}</h3>
            <p>${library.city || "Campus location"}</p>
            <p class="meta">${distanceText}</p>
            <p class="meta">Seats: ${library.total_seats ?? "n/a"}</p>
        `;
        container.appendChild(card);
    });
}

function renderMap() {
    if (state.role !== "user") {
        return;
    }

    const mapElement = document.getElementById("map");
    if (!mapElement || !window.L) {
        return;
    }

    const libraries = state.nearbyLibraries.length ? state.nearbyLibraries : state.libraries;
    const firstLibrary = libraries[0];
    const initialLat = state.location?.lat || firstLibrary?.lat || 30.3165;
    const initialLng = state.location?.lng || firstLibrary?.lng || 78.0322;

    if (!state.map) {
        state.map = L.map("map").setView([initialLat, initialLng], 13);
        L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
            attribution: "&copy; OpenStreetMap contributors",
        }).addTo(state.map);
        state.markersLayer = L.layerGroup().addTo(state.map);
    }

    state.markersLayer.clearLayers();
    libraries.forEach((library) => {
        L.marker([library.lat, library.lng])
            .addTo(state.markersLayer)
            .bindPopup(`${library.name}${library.distance_km !== undefined ? `<br>${library.distance_km} km away` : ""}`);
    });

    if (state.location) {
        state.userMarker = L.marker([state.location.lat, state.location.lng])
            .addTo(state.markersLayer)
            .bindPopup("You are here");
        state.map.setView([state.location.lat, state.location.lng], 13);
    } else if (firstLibrary) {
        state.map.setView([firstLibrary.lat, firstLibrary.lng], 13);
    }
}

function renderManagedLibrarySummary() {
    const name = document.getElementById("managedLibraryName");
    const meta = document.getElementById("managedLibraryMeta");
    if (!name || !meta) {
        return;
    }

    const library = getManagedLibrary();
    if (!library) {
        name.textContent = "No linked library found";
        meta.textContent = "This account is not attached to a library record yet.";
        return;
    }

    name.textContent = library.name;
    meta.textContent = `${library.city || "Campus location"} | Seats: ${library.total_seats ?? "n/a"} | Library ID: ${library.id}`;
}

function renderLibraryBooks() {
    const container = document.getElementById("libraryBookList");
    if (!container) {
        return;
    }

    container.innerHTML = "";
    if (state.libraryBooks.length === 0) {
        container.innerHTML = '<div class="card"><p>No books are linked to this library yet.</p></div>';
        return;
    }

    state.libraryBooks.forEach((book) => {
        const card = document.createElement("div");
        card.className = "card";
        card.innerHTML = `
            <h3>${book.title}</h3>
            <p>${book.author || "Unknown"} | ${book.genre || "General"}</p>
            <p class="meta">Total copies: ${book.total_copies ?? 0} | Available: ${book.available_copies ?? 0}</p>
            <div class="actions">
                <button class="secondary-btn" onclick="editLibraryBook('${book.id}')">Edit</button>
                <button class="danger-btn" onclick="deleteLibraryBook('${book.id}')">Delete</button>
            </div>
        `;
        container.appendChild(card);
    });
}

function renderLibraryBookings() {
    const container = document.getElementById("libraryBookingList");
    if (!container) {
        return;
    }

    container.innerHTML = "";
    if (state.libraryBookings.length === 0) {
        container.innerHTML = '<div class="card"><p>No bookings are assigned to this library yet.</p></div>';
        return;
    }

    state.libraryBookings.forEach((booking) => {
        const card = document.createElement("div");
        card.className = "card";
        card.innerHTML = `
            <h3>${booking.user_name || booking.user_email}</h3>
            <p>${bookingDetailLine(booking)}</p>
            <p class="meta">${booking.category} | ${booking.user_role || "user"}</p>
            <span class="status-pill">${booking.status || "active"}</span>
            <div class="actions">
                <button class="secondary-btn" onclick="editLibraryBooking('${booking.id}')">Edit</button>
                <button class="danger-btn" onclick="deleteLibraryBooking('${booking.id}')">Delete</button>
            </div>
        `;
        container.appendChild(card);
    });
}

function renderAdminLibraries() {
    const container = document.getElementById("adminLibraryList");
    if (!container) {
        return;
    }

    container.innerHTML = "";
    if (state.libraries.length === 0) {
        container.innerHTML = '<div class="card"><p>No libraries are available yet.</p></div>';
        return;
    }

    state.libraries.forEach((library) => {
        const bookCount = state.adminBooks.filter((book) => book.library_id === library.id).length;
        const card = document.createElement("div");
        card.className = "card";
        card.innerHTML = `
            <h3>${library.name}</h3>
            <p>${library.city || "Campus location"}</p>
            <p class="meta">ID: ${library.id} | Books: ${bookCount} | Seats: ${library.total_seats ?? "n/a"}</p>
            <p class="meta">Lat: ${library.lat} | Lng: ${library.lng}</p>
            <div class="actions">
                <button class="secondary-btn" onclick="editAdminLibrary('${library.id}')">Edit</button>
                <button class="danger-btn" onclick="deleteAdminLibrary('${library.id}')">Delete</button>
            </div>
        `;
        container.appendChild(card);
    });
}

function renderAdminBooks() {
    const container = document.getElementById("systemBookList");
    if (!container) {
        return;
    }

    container.innerHTML = "";
    if (state.adminBooks.length === 0) {
        container.innerHTML = '<div class="card"><p>No books are stored in the network yet.</p></div>';
        return;
    }

    state.adminBooks.forEach((book) => {
        const card = document.createElement("div");
        card.className = "card";
        card.innerHTML = `
            <h3>${book.title}</h3>
            <p>${book.author || "Unknown"} | ${book.genre || "General"}</p>
            <p class="meta">${book.library || "Unassigned"} | Available: ${book.available_copies ?? 0}/${book.total_copies ?? 0}</p>
        `;
        container.appendChild(card);
    });
}

function renderAccountList(containerId, people, emptyMessage) {
    const container = document.getElementById(containerId);
    if (!container) {
        return;
    }

    container.innerHTML = "";
    if (people.length === 0) {
        container.innerHTML = `<div class="card"><p>${emptyMessage}</p></div>`;
        return;
    }

    people.forEach((person) => {
        const card = document.createElement("div");
        card.className = "card";
        card.innerHTML = `
            <h3>${formatName(person)}</h3>
            <p>${person.email}</p>
            <p class="meta">Code: ${person.code || "n/a"}${person.library_id ? ` | Library: ${person.library_id}` : ""}</p>
        `;
        container.appendChild(card);
    });
}

function renderAccounts() {
    renderAccountList("adminAccountList", state.accounts.admins, "No admin accounts yet.");
    renderAccountList("userAccountList", state.accounts.users, "No user accounts yet.");
    renderAccountList("libraryAccountList", state.accounts.libraries, "No library manager accounts yet.");
}

function renderAdminBookings() {
    const container = document.getElementById("adminBookingList");
    if (!container) {
        return;
    }

    container.innerHTML = "";
    if (state.adminBookings.length === 0) {
        container.innerHTML = '<div class="card"><p>No system bookings yet.</p></div>';
        return;
    }

    state.adminBookings.forEach((booking) => {
        const card = document.createElement("div");
        card.className = "card";
        card.innerHTML = `
            <h3>${booking.user_name || booking.user_email}</h3>
            <p>${bookingDetailLine(booking)}</p>
            <p class="meta">${booking.category} | ${booking.user_role || "user"} | ${booking.user_email}</p>
            <span class="status-pill">${booking.status || "active"}</span>
            <div class="actions">
                <button class="secondary-btn" onclick="editAdminBooking('${booking.id}')">Edit</button>
                <button class="danger-btn" onclick="deleteAdminBooking('${booking.id}')">Delete</button>
            </div>
        `;
        container.appendChild(card);
    });
}

async function loadLibraries() {
    const payload = await requestJson("/api/libraries");
    state.libraries = payload.items || [];
    renderSeatLibraryOptions();
    renderNearbyLibraries();
    renderManagedLibrarySummary();
    renderAdminLibraries();
}

async function loadUserBooks(searchTerm = "") {
    state.currentQuery = searchTerm;
    const query = searchTerm ? `?q=${encodeURIComponent(searchTerm)}` : "";
    const payload = await requestJson(`/api/books${query}`);
    state.userBooks = payload.items || [];
    renderUserBooks();
}

async function loadUserBookings() {
    const payload = await requestJson(`/api/bookings?email=${encodeURIComponent(state.session.email)}`);
    state.userBookings = payload.items || [];
    renderUserBookings();
}

async function loadLibraryBooks() {
    if (!state.managedLibraryId) {
        state.libraryBooks = [];
        renderLibraryBooks();
        return;
    }

    const payload = await requestJson(`/api/books?library_id=${encodeURIComponent(state.managedLibraryId)}`);
    state.libraryBooks = payload.items || [];
    renderLibraryBooks();
}

async function loadLibraryBookings() {
    if (!state.managedLibraryId) {
        state.libraryBookings = [];
        renderLibraryBookings();
        return;
    }

    const payload = await requestJson(`/api/bookings?library_id=${encodeURIComponent(state.managedLibraryId)}`);
    state.libraryBookings = payload.items || [];
    renderLibraryBookings();
}

async function loadAdminBooks() {
    const payload = await requestJson("/api/books");
    state.adminBooks = payload.items || [];
    renderAdminBooks();
    renderAdminLibraries();
}

async function loadAdminBookings() {
    const payload = await requestJson("/api/bookings");
    state.adminBookings = payload.items || [];
    renderAdminBookings();
}

async function loadAccounts() {
    const [admins, users, libraries] = await Promise.all([
        requestJson("/api/admins"),
        requestJson("/api/users"),
        requestJson("/api/library-accounts"),
    ]);

    state.accounts.admins = admins.items || [];
    state.accounts.users = users.items || [];
    state.accounts.libraries = libraries.items || [];
    renderAccounts();
}

function getCurrentPosition() {
    return new Promise((resolve, reject) => {
        if (!navigator.geolocation) {
            reject(new Error("Geolocation is not supported in this browser."));
            return;
        }

        navigator.geolocation.getCurrentPosition(resolve, reject, {
            enableHighAccuracy: true,
            timeout: 10000,
        });
    });
}

async function loadNearbyLibraries() {
    try {
        const position = await getCurrentPosition();
        state.location = {
            lat: position.coords.latitude,
            lng: position.coords.longitude,
        };

        const payload = await requestJson(
            `/api/libraries/nearby?lat=${encodeURIComponent(state.location.lat)}&lng=${encodeURIComponent(state.location.lng)}`
        );
        state.nearbyLibraries = payload.items || [];
    } catch (error) {
        state.nearbyLibraries = [];
    }

    renderNearbyLibraries();
    renderMap();
}

async function searchBooks() {
    try {
        setStatus("Searching books in the connected libraries...");
        await loadUserBooks(document.getElementById("searchInput").value.trim());
        updateOverviewStats();
        setStatus("Search complete.");
    } catch (error) {
        setStatus(error.message, true);
    }
}

async function bookSeat() {
    const libraryId = document.getElementById("seatLibrary").value;
    const date = document.getElementById("seatDate").value;
    const time = document.getElementById("seatTime").value;

    if (!libraryId || !date || !time) {
        setStatus("Choose a library, date, and time before booking.", true);
        return;
    }

    const library = state.libraries.find((item) => item.id === libraryId);
    if (!library) {
        setStatus("Selected library was not found.", true);
        return;
    }

    try {
        await requestJson("/api/bookings", {
            method: "POST",
            body: JSON.stringify({
                user_email: state.session.email,
                user_name: formatName(),
                user_role: state.role,
                category: "seat",
                title: "Study Seat",
                library_id: libraryId,
                library: library.name,
                date,
                time,
                status: "active",
            }),
        });

        document.getElementById("seatLibrary").value = "";
        document.getElementById("seatDate").value = "";
        document.getElementById("seatTime").value = "";

        await loadUserBookings();
        updateOverviewStats();
        setStatus("Seat booked successfully.");
        scrollToSection("myBookings");
    } catch (error) {
        setStatus(error.message, true);
    }
}

async function rentBook(bookId) {
    const book = state.userBooks.find((item) => item.id === bookId);
    if (!book) {
        setStatus("Selected book was not found.", true);
        return;
    }

    try {
        await requestJson("/api/bookings", {
            method: "POST",
            body: JSON.stringify({
                user_email: state.session.email,
                user_name: formatName(),
                user_role: state.role,
                category: "rental",
                title: book.title,
                book_id: book.id,
                status: "active",
            }),
        });

        await Promise.all([loadUserBooks(state.currentQuery), loadUserBookings()]);
        updateOverviewStats();
        setStatus(`"${book.title}" rented successfully.`);
        scrollToSection("myBookings");
    } catch (error) {
        setStatus(error.message, true);
    }
}

function findBooking(collection, bookingId) {
    return collection.find((booking) => booking.id === bookingId) || null;
}

async function updateBooking(bookingId, payload, reloadFn) {
    await requestJson(`/api/bookings/${bookingId}`, {
        method: "PUT",
        body: JSON.stringify(payload),
    });

    await reloadFn();
}

async function deleteBooking(bookingId, reloadFn) {
    await requestJson(`/api/bookings/${bookingId}`, {
        method: "DELETE",
    });

    await reloadFn();
}

async function editUserBooking(bookingId) {
    const booking = findBooking(state.userBookings, bookingId);
    if (!booking) {
        setStatus("Booking not found.", true);
        return;
    }

    const status = window.prompt("Status", booking.status || "active");
    if (status === null) {
        return;
    }

    const payload = { status: status.trim() || booking.status || "active" };
    if (booking.category === "seat") {
        const date = window.prompt("Date (YYYY-MM-DD)", booking.date || "");
        const time = window.prompt("Time slot", booking.time || "");
        if (date === null || time === null) {
            return;
        }
        payload.library_id = booking.library_id;
        payload.date = date.trim();
        payload.time = time.trim();
    } else {
        const title = window.prompt("Book title", booking.title || "");
        if (title === null) {
            return;
        }
        payload.title = title.trim();
    }

    try {
        await updateBooking(bookingId, payload, async () => {
            await Promise.all([loadUserBooks(state.currentQuery), loadUserBookings()]);
        });
        updateOverviewStats();
        setStatus("Booking updated.");
    } catch (error) {
        setStatus(error.message, true);
    }
}

async function deleteUserBooking(bookingId) {
    if (!window.confirm("Delete this booking?")) {
        return;
    }

    try {
        await deleteBooking(bookingId, async () => {
            await Promise.all([loadUserBooks(state.currentQuery), loadUserBookings()]);
        });
        updateOverviewStats();
        setStatus("Booking deleted.");
    } catch (error) {
        setStatus(error.message, true);
    }
}

function resetBookForm() {
    state.editingBookId = "";
    document.getElementById("libraryBookFormTitle").textContent = "Add Book";
    document.getElementById("bookSubmitBtn").textContent = "Save Book";
    document.getElementById("bookTitleInput").value = "";
    document.getElementById("bookAuthorInput").value = "";
    document.getElementById("bookGenreInput").value = "";
    document.getElementById("bookTotalCopiesInput").value = "1";
    document.getElementById("bookAvailableCopiesInput").value = "1";
}

async function submitLibraryBook() {
    if (!state.managedLibraryId) {
        setStatus("This account is not linked to a library yet.", true);
        return;
    }

    const payload = {
        title: document.getElementById("bookTitleInput").value.trim(),
        author: document.getElementById("bookAuthorInput").value.trim(),
        genre: document.getElementById("bookGenreInput").value.trim(),
        total_copies: document.getElementById("bookTotalCopiesInput").value.trim(),
        available_copies: document.getElementById("bookAvailableCopiesInput").value.trim(),
        library_id: state.managedLibraryId,
    };

    if (!payload.title || !payload.author) {
        setStatus("Title and author are required for books.", true);
        return;
    }

    const path = state.editingBookId ? `/api/books/${state.editingBookId}` : "/api/books";
    const method = state.editingBookId ? "PUT" : "POST";
    const isEditing = Boolean(state.editingBookId);

    try {
        await requestJson(path, {
            method,
            body: JSON.stringify(payload),
        });

        resetBookForm();
        await loadLibraryBooks();
        updateOverviewStats();
        setStatus(isEditing ? "Book updated." : "Book created.");
    } catch (error) {
        setStatus(error.message, true);
    }
}

function editLibraryBook(bookId) {
    const book = state.libraryBooks.find((item) => item.id === bookId);
    if (!book) {
        setStatus("Book not found.", true);
        return;
    }

    state.editingBookId = bookId;
    document.getElementById("libraryBookFormTitle").textContent = "Edit Book";
    document.getElementById("bookSubmitBtn").textContent = "Update Book";
    document.getElementById("bookTitleInput").value = book.title || "";
    document.getElementById("bookAuthorInput").value = book.author || "";
    document.getElementById("bookGenreInput").value = book.genre || "";
    document.getElementById("bookTotalCopiesInput").value = String(book.total_copies ?? 1);
    document.getElementById("bookAvailableCopiesInput").value = String(book.available_copies ?? 1);
    scrollToSection("libraryInventory");
}

async function deleteLibraryBook(bookId) {
    if (!window.confirm("Delete this book from the library inventory?")) {
        return;
    }

    try {
        await requestJson(`/api/books/${bookId}`, {
            method: "DELETE",
        });
        await loadLibraryBooks();
        updateOverviewStats();
        setStatus("Book deleted.");
    } catch (error) {
        setStatus(error.message, true);
    }
}

async function editLibraryBooking(bookingId) {
    const booking = findBooking(state.libraryBookings, bookingId);
    if (!booking) {
        setStatus("Booking not found.", true);
        return;
    }

    const status = window.prompt("Status", booking.status || "active");
    if (status === null) {
        return;
    }

    const payload = { status: status.trim() || booking.status || "active" };
    if (booking.category === "seat") {
        const date = window.prompt("Date (YYYY-MM-DD)", booking.date || "");
        const time = window.prompt("Time slot", booking.time || "");
        if (date === null || time === null) {
            return;
        }
        payload.library_id = booking.library_id;
        payload.date = date.trim();
        payload.time = time.trim();
    } else {
        const title = window.prompt("Book title", booking.title || "");
        if (title === null) {
            return;
        }
        payload.title = title.trim();
    }

    try {
        await updateBooking(bookingId, payload, async () => {
            await Promise.all([loadLibraryBooks(), loadLibraryBookings()]);
        });
        updateOverviewStats();
        setStatus("Library booking updated.");
    } catch (error) {
        setStatus(error.message, true);
    }
}

async function deleteLibraryBooking(bookingId) {
    if (!window.confirm("Delete this library booking?")) {
        return;
    }

    try {
        await deleteBooking(bookingId, async () => {
            await Promise.all([loadLibraryBooks(), loadLibraryBookings()]);
        });
        updateOverviewStats();
        setStatus("Library booking deleted.");
    } catch (error) {
        setStatus(error.message, true);
    }
}

function resetLibraryForm() {
    state.editingLibraryId = "";
    document.getElementById("adminLibraryFormTitle").textContent = "Add Library";
    document.getElementById("librarySubmitBtn").textContent = "Save Library";
    document.getElementById("libraryNameInput").value = "";
    document.getElementById("libraryCityInput").value = "";
    document.getElementById("libraryLatInput").value = "30.3165";
    document.getElementById("libraryLngInput").value = "78.0322";
    document.getElementById("librarySeatsInput").value = "20";
}

async function submitAdminLibrary() {
    const payload = {
        name: document.getElementById("libraryNameInput").value.trim(),
        city: document.getElementById("libraryCityInput").value.trim(),
        lat: document.getElementById("libraryLatInput").value.trim(),
        lng: document.getElementById("libraryLngInput").value.trim(),
        total_seats: document.getElementById("librarySeatsInput").value.trim(),
    };

    if (!payload.name) {
        setStatus("Library name is required.", true);
        return;
    }

    const path = state.editingLibraryId ? `/api/libraries/${state.editingLibraryId}` : "/api/libraries";
    const method = state.editingLibraryId ? "PUT" : "POST";
    const isEditing = Boolean(state.editingLibraryId);

    try {
        await requestJson(path, {
            method,
            body: JSON.stringify(payload),
        });

        resetLibraryForm();
        await Promise.all([loadLibraries(), loadAdminBooks()]);
        updateOverviewStats();
        setStatus(isEditing ? "Library updated." : "Library created.");
    } catch (error) {
        setStatus(error.message, true);
    }
}

function editAdminLibrary(libraryId) {
    const library = state.libraries.find((item) => item.id === libraryId);
    if (!library) {
        setStatus("Library not found.", true);
        return;
    }

    state.editingLibraryId = libraryId;
    document.getElementById("adminLibraryFormTitle").textContent = "Edit Library";
    document.getElementById("librarySubmitBtn").textContent = "Update Library";
    document.getElementById("libraryNameInput").value = library.name || "";
    document.getElementById("libraryCityInput").value = library.city || "";
    document.getElementById("libraryLatInput").value = String(library.lat ?? "");
    document.getElementById("libraryLngInput").value = String(library.lng ?? "");
    document.getElementById("librarySeatsInput").value = String(library.total_seats ?? 20);
    scrollToSection("adminLibraries");
}

async function deleteAdminLibrary(libraryId) {
    if (!window.confirm("Delete this library?")) {
        return;
    }

    try {
        await requestJson(`/api/libraries/${libraryId}`, {
            method: "DELETE",
        });
        await Promise.all([loadLibraries(), loadAdminBooks()]);
        updateOverviewStats();
        setStatus("Library deleted.");
    } catch (error) {
        setStatus(error.message, true);
    }
}

async function editAdminBooking(bookingId) {
    const booking = findBooking(state.adminBookings, bookingId);
    if (!booking) {
        setStatus("Booking not found.", true);
        return;
    }

    const status = window.prompt("Status", booking.status || "active");
    if (status === null) {
        return;
    }

    const payload = { status: status.trim() || booking.status || "active" };
    if (booking.category === "seat") {
        const date = window.prompt("Date (YYYY-MM-DD)", booking.date || "");
        const time = window.prompt("Time slot", booking.time || "");
        if (date === null || time === null) {
            return;
        }
        payload.library_id = booking.library_id;
        payload.date = date.trim();
        payload.time = time.trim();
    } else {
        const title = window.prompt("Book title", booking.title || "");
        if (title === null) {
            return;
        }
        payload.title = title.trim();
    }

    try {
        await updateBooking(bookingId, payload, async () => {
            await Promise.all([loadAdminBookings(), loadAdminBooks()]);
        });
        updateOverviewStats();
        setStatus("System booking updated.");
    } catch (error) {
        setStatus(error.message, true);
    }
}

async function deleteAdminBooking(bookingId) {
    if (!window.confirm("Delete this booking?")) {
        return;
    }

    try {
        await deleteBooking(bookingId, async () => {
            await Promise.all([loadAdminBookings(), loadAdminBooks()]);
        });
        updateOverviewStats();
        setStatus("System booking deleted.");
    } catch (error) {
        setStatus(error.message, true);
    }
}

function attachEventHandlers() {
    document.getElementById("navSearch").addEventListener("click", () => scrollToSection("bookSearch"));
    document.getElementById("navMap").addEventListener("click", () => scrollToSection("nearbyLibraries"));
    document.getElementById("navBookings").addEventListener("click", () => scrollToSection("myBookings"));
    document.getElementById("navLibrary").addEventListener("click", () => scrollToSection("libraryInventory"));
    document.getElementById("navAdmin").addEventListener("click", () => scrollToSection("adminLibraries"));
    document.getElementById("logoutBtn").addEventListener("click", logout);

    document.getElementById("searchBtn").addEventListener("click", searchBooks);
    document.getElementById("searchInput").addEventListener("keydown", (event) => {
        if (event.key === "Enter") {
            searchBooks();
        }
    });

    document.getElementById("seatBookBtn").addEventListener("click", bookSeat);
    document.getElementById("bookSubmitBtn").addEventListener("click", submitLibraryBook);
    document.getElementById("bookResetBtn").addEventListener("click", resetBookForm);
    document.getElementById("librarySubmitBtn").addEventListener("click", submitAdminLibrary);
    document.getElementById("libraryResetBtn").addEventListener("click", resetLibraryForm);
}

async function initializeRoleView() {
    await loadLibraries();

    if (state.role === "admin") {
        await Promise.all([loadAdminBooks(), loadAdminBookings(), loadAccounts()]);
    } else if (state.role === "lib") {
        await Promise.all([loadLibraryBooks(), loadLibraryBookings()]);
    } else {
        await Promise.all([loadUserBooks(), loadUserBookings()]);
        await loadNearbyLibraries();
    }

    renderMap();
    updateOverviewStats();
}

async function initializeDashboard() {
    if (!ensureSession()) {
        return;
    }

    showRolePanels();
    attachEventHandlers();
    populateHeader();
    document.getElementById("seatDate").min = new Date().toISOString().split("T")[0];
    resetBookForm();
    resetLibraryForm();
    setStatus("Loading live data from the C++ backend...");

    try {
        await initializeRoleView();
        populateHeader();
        setStatus("Frontend connected to the C++ backend successfully.");
    } catch (error) {
        setStatus(error.message, true);
    }
}

window.scrollToSection = scrollToSection;
window.logout = logout;
window.rentBook = rentBook;
window.bookSeat = bookSeat;
window.editUserBooking = editUserBooking;
window.deleteUserBooking = deleteUserBooking;
window.editLibraryBook = editLibraryBook;
window.deleteLibraryBook = deleteLibraryBook;
window.editLibraryBooking = editLibraryBooking;
window.deleteLibraryBooking = deleteLibraryBooking;
window.editAdminLibrary = editAdminLibrary;
window.deleteAdminLibrary = deleteAdminLibrary;
window.editAdminBooking = editAdminBooking;
window.deleteAdminBooking = deleteAdminBooking;

document.addEventListener("DOMContentLoaded", initializeDashboard);
