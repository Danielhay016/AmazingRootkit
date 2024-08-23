// Handle login form submission - Login.html
document.getElementById('login-form').onsubmit = function(event) {
    event.preventDefault();
    var username = document.getElementById('username').value;
    var password = document.getElementById('password').value;

    fetch('/login', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ username: username, password: password })
    })
    .then(response => {
        if (response.ok) {
            return response.json();
        } else {
            throw new Error('Login failed');
        }
    })
    .then(data => {
        if (data.status === 'success') {
            window.location.href = '/dashboard'; // Redirect on successful login
        } else {
            document.getElementById('login-error').style.display = 'block'; // Show error
        }
    })
    .catch(error => {
        document.getElementById('login-error').style.display = 'block'; // Show error
    });
};

// Handle logout button click - related to all html files
document.getElementById('logout').onclick = function(event) {
    event.preventDefault(); // Prevent default action
    fetch('/logout', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' }
    })
    .then(response => {
        if (response.ok) {
            window.location.href = '/login'; // Redirect on successful logout
        } else {
            throw new Error('Logout failed');
        }
    })
    .catch(error => {
        console.error('Logout error:', error);
    });
};


