const API_BASE = "http://127.0.0.1:8000";
const SESSION_KEY = "findmybookSession";

const state = {
    libraries: [],
};

function setStatus(message, type = "") {
    const status = document.getElementById("authStatus");
    status.textContent = message;
    status.className = `status ${type}`.trim();
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

function deriveLibraryIdFromCode(code = "") {
    if (!/^l\d+$/.test(code)) {
        return "";
    }

    return `lib-${code.slice(1)}`;
}

function storeSession(role, person) {
    localStorage.setItem(SESSION_KEY, JSON.stringify({
        role,
        ...person,
        library_id: person.library_id || deriveLibraryIdFromCode(person.code),
    }));
}

function toggleField(wrapperId, visible) {
    const element = document.getElementById(wrapperId);
    if (!element) {
        return;
    }

    element.style.display = visible ? "block" : "none";
}

function populateLibrarySelect() {
    const select = document.getElementById("signupLibrary");
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

function updateLoginRoleFields() {
    const role = document.getElementById("loginRole").value;
    toggleField("loginAdminKeyWrap", role === "admin");
}

function updateSignupRoleFields() {
    const role = document.getElementById("signupRole").value;
    toggleField("signupAdminKeyWrap", role === "admin");
    toggleField("signupLibraryWrap", role === "lib");
}

async function loadLibraries() {
    try {
        const payload = await requestJson("/api/libraries");
        state.libraries = payload.items || [];
        populateLibrarySelect();
    } catch (error) {
        setStatus(`Library list could not be loaded: ${error.message}`, "error");
    }
}

document.addEventListener("DOMContentLoaded", () => {
    const existingSession = localStorage.getItem(SESSION_KEY);
    if (existingSession) {
        window.location.href = "main.html";
        return;
    }

    const loginBox = document.getElementById("loginBox");
    const signupBox = document.getElementById("signupBox");

    document.getElementById("showSignup").addEventListener("click", () => {
        loginBox.style.display = "none";
        signupBox.style.display = "block";
        setStatus("");
    });

    document.getElementById("showLogin").addEventListener("click", () => {
        signupBox.style.display = "none";
        loginBox.style.display = "block";
        setStatus("");
    });

    document.getElementById("loginRole").addEventListener("change", updateLoginRoleFields);
    document.getElementById("signupRole").addEventListener("change", updateSignupRoleFields);

    updateLoginRoleFields();
    updateSignupRoleFields();
    loadLibraries();

    document.getElementById("signupBtn").addEventListener("click", async () => {
        const role = document.getElementById("signupRole").value;
        const fname = document.getElementById("signupFname").value.trim();
        const lname = document.getElementById("signupLname").value.trim();
        const email = document.getElementById("signupEmail").value.trim();
        const password = document.getElementById("signupPass").value.trim();
        const adminKey = document.getElementById("signupAdminKey").value.trim();
        const libraryId = document.getElementById("signupLibrary").value;

        if (!fname || !lname || !email || !password) {
            setStatus("Please fill all signup fields.", "error");
            return;
        }
        if (role === "admin" && !adminKey) {
            setStatus("Admin signup requires the admin key.", "error");
            return;
        }
        if (role === "lib" && !libraryId) {
            setStatus("Choose which library this manager account belongs to.", "error");
            return;
        }

        try {
            setStatus("Creating account...");
            const payload = await requestJson("/api/signup", {
                method: "POST",
                body: JSON.stringify({
                    role,
                    fname,
                    lname,
                    email,
                    password,
                    admin_key: adminKey,
                    library_id: libraryId,
                }),
            });

            storeSession(role, payload.person);
            setStatus("Signup successful. Redirecting...", "success");
            window.location.href = "main.html";
        } catch (error) {
            setStatus(error.message, "error");
        }
    });

    document.getElementById("loginBtn").addEventListener("click", async () => {
        const role = document.getElementById("loginRole").value;
        const email = document.getElementById("loginEmail").value.trim();
        const password = document.getElementById("loginPass").value.trim();
        const adminKey = document.getElementById("loginAdminKey").value.trim();

        if (!email || !password) {
            setStatus("Please enter email and password.", "error");
            return;
        }
        if (role === "admin" && !adminKey) {
            setStatus("Admin login requires the admin key.", "error");
            return;
        }

        try {
            setStatus("Logging in...");
            const payload = await requestJson("/api/login", {
                method: "POST",
                body: JSON.stringify({
                    role,
                    email,
                    password,
                    admin_key: adminKey,
                }),
            });

            storeSession(role, payload.person);
            setStatus("Login successful. Redirecting...", "success");
            window.location.href = "main.html";
        } catch (error) {
            setStatus(error.message, "error");
        }
    });
});
