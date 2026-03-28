document.addEventListener("DOMContentLoaded", () => {
    let users = JSON.parse(localStorage.getItem("users")) || [];

    const loginBox = document.getElementById("loginBox");
    const signupBox = document.getElementById("signupBox");

    // Show signup form
    document.querySelector("#loginBox span").addEventListener("click", () => {
        loginBox.style.display = "none";
        signupBox.style.display = "block";
    });

    // Show login form
    document.querySelector("#signupBox span").addEventListener("click", () => {
        signupBox.style.display = "none";
        loginBox.style.display = "block";
    });

    // Signup button
    document.querySelector("#signupBox button").addEventListener("click", () => {
        const u = document.getElementById('signupUser').value;
        const p = document.getElementById('signupPass').value;
        const c = document.getElementById('signupContact').value;

        if (!u || !p || !c) {
            alert("Please fill all fields!");
            return;
        }

        if(users.find(x => x.username === u)){
            alert("User already exists!");
            return;
        }

        users.push({username:u, password:p, contact:c});
        localStorage.setItem("users", JSON.stringify(users));
        alert("Signup successful! Redirecting...");

        window.location.href = "main.html?user=" + encodeURIComponent(u);
    });

    // Login button
    document.querySelector("#loginBox button").addEventListener("click", () => {
        const u = document.getElementById('loginUser').value;
        const p = document.getElementById('loginPass').value;

        const found = users.find(x => x.username === u && x.password === p);
        if(!found){
            alert("Invalid username or password!");
            return;
        }

        window.location.href = "main.html?user=" + encodeURIComponent(u);
    });
});