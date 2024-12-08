import { useState, useEffect } from "react";
import toast from "react-hot-toast";
import axios from 'axios';
import { useNavigate } from 'react-router-dom';

const BASE_URL = "http://localhost:8080/api"

type Props = {
  isOpen: boolean;
  setIsOpen: React.Dispatch<React.SetStateAction<boolean>>;
};

const RoomList: React.FC<Props> = ({ isOpen, setIsOpen }) => {
  const [loading, setLoading] = useState(false);
  const [rooms, setRooms] = useState([]);
  const navigate = useNavigate();

  useEffect(() => {
    const fetchRooms = async () => {
      setLoading(true);
      try {
        const res = await axios.get(`${BASE_URL}/room/fetch_all_room`, {
          headers: {
            'Content-Type': 'application/json',
          },
          withCredentials: true
        });
        setRooms(res.data.rooms);
      } catch (err: any) {
        console.log(err);
        if (err.message) {
          toast.error(err.message);
        }
      } finally {
        setLoading(false);
      }
    };

    if (isOpen) {
      fetchRooms();
    }
  }, [isOpen]);

  const handleJoinRoom = (room_name: string) => {
    navigate(`/room/${room_name}`);
  };

  return (
    <>
      {isOpen && (
        <div
          onClick={() => setIsOpen(false)}
          className="flex bg-black/30 backdrop-blur-[2px] overflow-y-auto overflow-x-hidden fixed top-0 right-0 left-0 z-50 justify-center items-center w-full md:inset-0 h-full max-h-full"
        >
          <div
            onClick={(e) => e.stopPropagation()}
            className="relative p-4 w-full max-w-4xl max-h-full"
          >
            {/* Modal content */}
            <div className="relative rounded-lg shadow bg-gray-800/90 border-[1px] border-white">
              {/* Modal header */}
              <div className="flex items-center justify-between p-4 md:p-6 border-b rounded-t border-gray-600">
                <h3 className="text-3xl font-semibold text-white">
                  List of waiting rooms
                </h3>
                <button
                  onClick={() => setIsOpen(false)}
                  type="button"
                  className="end-2.5 text-gray-400 bg-transparent rounded-lg text-lg w-8 h-8 ms-auto inline-flex justify-center items-center hover:bg-gray-600 hover:text-white"
                  data-modal-hide="authentication-modal"
                >
                  <svg
                    className="w-3 h-3"
                    aria-hidden="true"
                    xmlns="http://www.w3.org/2000/svg"
                    fill="none"
                    viewBox="0 0 14 14"
                  >
                    <path
                      stroke="currentColor"
                      strokeLinecap="round"
                      strokeLinejoin="round"
                      strokeWidth="2"
                      d="m1 1 6 6m0 0 6 6M7 7l6-6M7 7l-6 6"
                    />
                  </svg>
                  <span className="sr-only">Close modal</span>
                </button>
              </div>
              {/* Modal body */}
              <div className="p-4 md:px-6 md:py-4 mb-2 max-h-[70vh] overflow-y-scroll">
                {loading ? (
                  <div className="text-white text-center">Loading...</div>
                ) : (
                  <div className="grid grid-cols-1 md:grid-cols-2 gap-6 space-y-2 sm:space-y-0">
                    {rooms.map((room: any) => (
                      <div key={room.room_name} className="flex justify-between items-center bg-gray-700 p-4 rounded-lg">
                        <div>
                          <div className="text-xl font-semibold text-white mb-1">{room.room_name}</div>
                          <div className="text-base text-gray-100 w-56 truncate">Host: {room.host}</div>
                          <div className="text-sm text-gray-100 w-56 truncate">Capacity: {room.capacity}</div>
                          <div className="text-sm text-gray-300 w-56 truncate">Players: {room.users.map((user: any) => user.username).join(", ")}</div>
                        </div>
                        <button
                          onClick={() => handleJoinRoom(room.room_name)}
                          className="text-lg text-white font-semibold px-6 py-2 rounded-full border-2 hover:bg-white hover:text-black transition duration-300"
                        >
                          Join
                        </button>
                      </div>
                    ))}
                  </div>
                )}
              </div>
            </div>
          </div>
        </div>
      )}
    </>
  );
};

export default RoomList;