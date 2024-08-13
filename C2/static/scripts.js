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

// Handle command form submission - Campaigns.html
document.getElementById('command-form').onsubmit = function(event) {
    event.preventDefault();
    var command = document.getElementById('command').value;

    fetch('/send_command', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ command: command })
    })
    .then(response => response.json())
    .then(data => {
        document.getElementById('results').style.display = 'block'; // Show results
        document.getElementById('results-content').innerText = data.response; // Display result
    })
    .catch(error => {
        console.error('Error:', error);
        document.getElementById('results').style.display = 'block'; // Show results
        document.getElementById('results-content').innerText = 'An error occurred.'; // Display error message
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

// Handle generate report button click - Dashboard.html
document.getElementById('generate-report').onclick = function(event) {
    event.preventDefault(); // Prevent default action

    // Send a GET request to the /report endpoint
    fetch('/report')
        .then(response => {
            if (!response.ok) {
                throw new Error('Network response was not ok');
            }
            return response.blob(); // Return the response as a blob
        })
        .then(blob => {
            // Create a link element to download the file
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = 'exported_data.json'; // Specify the file name
            document.body.appendChild(a);
            a.click(); // Trigger the download
            a.remove(); // Remove the link element
            window.URL.revokeObjectURL(url); // Clean up the URL object
        })
        .catch(error => {
            console.error('Error generating report:', error); // Log errors
        });
};

 //Confirm Delete - Management.html handle it later 
function confirmDelete(campaignId) {
    if (confirm('Are you sure you want to delete this campaign?')) {
        fetch(`/delete_campaign/${campaignId}`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            }
        })
        .then(response => response.json())
        .then(data => {
            if (data.status === 'success') {
                document.getElementById(`campaign-row-${campaignId}`).remove();
            } else {
                alert('Failed to delete campaign: ' + data.message);
            }
        })
        .catch(error => {
            alert('Error: ' + error);
        });
    }
}

