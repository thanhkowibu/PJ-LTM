import { useState, useEffect } from 'react';
import axios from 'axios';

export const Home = () => {
  // State variables
  const [chatMessages, setChatMessages] = useState<string[]>([]);
  const [choice, setChoice] = useState<string | null>(null);
  const [chatInput, setChatInput] = useState<string>('');

  const SERVER_URL = window.location.hostname === 'localhost'
    ? import.meta.env.VITE_LOCAL_SERVER_URL
    : import.meta.env.VITE_GUEST_SERVER_URL;

    const fetchData = async () => {
      try {
        const response = await axios.get('http://127.0.0.1:8080/api/data', {
          headers: {
            'Content-Type': 'application/json',
          },
        });
        
        console.log('Server Response:', response.data);
    } catch (error) {
      console.error('Fetch error:', error);
    }
  };

  // Start Server-Sent Events (SSE)
  const startSSE = () => {
    const eventSource = new EventSource('http://127.0.0.1:8080/api/subscribe');
  
    eventSource.onmessage = (event) => {
      console.log('New message from server:', event.data);
      setChatMessages((prevMessages) => [...prevMessages, `Broadcast: ${event.data}`]);
    };
  
    eventSource.onerror = () => {
      console.error('SSE connection error. Reconnecting...');
      eventSource.close();
  
      // Retry connection after a delay
      setTimeout(startSSE, 5000);
    };
  };
  

  // Fetch data on component mount
  useEffect(() => {
    console.log('useEffect triggered');
    fetchData();
    startSSE();
  }, []);

  // Send a chat message
  const sendMessage = async (message: string) => {
    try {
      const response = await axios.post('http://127.0.0.1:8080/api/message', { message }, {
        headers: {
          'Content-Type': 'application/json',
        },
      });

      console.log(response.data);
      setChatMessages((prevMessages) => [...prevMessages, `Server: ${response.data.message}`]);
    } catch (error) {
      console.error('Error:', error);
    }
  };

  // Send a choice (high or low)
  const sendChoice = async (choice: string) => {
    try {
      const response = await axios.post('http://127.0.0.1:8080/api/choice', { choice }, {
        headers: {
          'Content-Type': 'application/json',
        },
      });

      console.log(response.data);
      setChatMessages((prevMessages) => [...prevMessages, `Server: ${response.data.choice}`]);
    } catch (error) {
      console.error('Error:', error);
    }
  };

  return (
    <div className="flex flex-col gap-8 p-8">
      <h1>Welcome to the Higher Lower game!</h1>
      <h2>Choices</h2>

      {/* Buttons for choices */}
      <div>
        <button onClick={() => sendChoice('1')}>Option 1</button>
        <button onClick={() => sendChoice('2')}>Option 2</button>
      </div>

      {/* Chat Box */}
      <div id="chat-box" style={{ border: '1px solid black', padding: '10px', height: '200px', overflowY: 'scroll' }}>
        {chatMessages.map((msg, index) => (
          <p key={index}>{msg}</p>
        ))}
      </div>

      {/* Chat Input */}
      <div>
        <input
          type="text"
          id="chat-input"
          placeholder="Send message"
          value={chatInput}
          onChange={(e) => setChatInput(e.target.value)}
          onKeyDown={(e) => {
            if (e.key === 'Enter') {
              sendMessage(chatInput);
              setChatInput('');
            }
          }}
        />
        <button
          onClick={() => {
            if (chatInput) {
              sendMessage(chatInput);
              setChatInput('');
            }
          }}
        >
          Send
        </button>
      </div>
    </div>
  );
};