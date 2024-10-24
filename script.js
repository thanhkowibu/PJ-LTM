// script.js
console.log("Script loaded");

function sendMessage() {
    var message = document.getElementById('chat-input').value;
    var messageObj = { message: message };
    
    console.log("Sending message:", messageObj); // Log the message object

    fetch('/send', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(messageObj)
    })
    .then(response => response.text())
    .then(data => {
        document.getElementById('chat-box').innerHTML += '<p>' + data + '</p>';
    })
    .catch(error => console.error('Error:', error));

    document.getElementById('chat-input').value = '';
}

function sendChoice(choice) {
    var choiceObj = { choice: choice };
    
    console.log("Sending choice:", choiceObj); // Log the choice object

    fetch('/send', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(choiceObj)
    })
    .then(response => response.text())
    .then(data => {
        document.getElementById('chat-box').innerHTML += '<p>' + data + '</p>';
    })
    .catch(error => console.error('Error:', error));
}
