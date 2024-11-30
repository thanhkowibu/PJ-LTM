// script.js
async function fetchData() {
    try {
        const response = await fetch('http://127.0.0.1:8080/api/data', {
            method: 'GET',
            headers: {
                'Content-Type': 'application/json'
            }
        });

        if (!response.ok) {
            throw new Error('Network response was not ok');
        }

        const data = await response.json(); // Parse JSON response
        console.log('Server Response:', data);

    } catch (error) {
        console.error('Fetch error:', error);
    }
}

// Call the fetchData function when the page loads or as needed
fetchData();

function startSSE() {
    const eventSource = new EventSource('http://127.0.0.1:8080/api/subscribe');

    eventSource.onmessage = function(event) {
        console.log('New message from server:', event.data);
        document.getElementById('chat-box').innerHTML += '<p>Broadcast: ' + event.data + '</p>';
    };

    eventSource.onerror = function() {
        console.error("SSE connection error. Reconnecting...");
    };
}

// Call the startSSE function to initiate the SSE connection
startSSE();

function sendMessage() {
    var message = document.getElementById('chat-input').value;
    var messageObj = { message: message };

    fetch('http://127.0.0.1:8080/api/message', {  // Update endpoint
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(messageObj)
    })
    .then(response => response.json())
    .then(data => {
        console.log(data);
        document.getElementById('chat-box').innerHTML += '<p>Server: ' + data.message + '</p>';
    })
    .catch(error => console.error('Error:', error));

    document.getElementById('chat-input').value = '';
}

function sendChoice(choice) {
    var choiceObj = { choice: choice };

    fetch('http://127.0.0.1:8080/api/choice', {  // Update endpoint
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(choiceObj)
    })
    .then(response => response.json())
    .then(data => {
        console.log(data);
        document.getElementById('chat-box').innerHTML += '<p>Server: ' + data.choice + '</p>';
    })
    .catch(error => console.error('Error:', error));
}
