import { useState, useEffect } from "react";
import { useParams } from "react-router-dom";
import toast from "react-hot-toast";
import axios from 'axios';

const BASE_URL = "http://localhost:8080/api"

export const WaitingRoom = () => {
  const { id } = useParams<{ id: string }>();
  const [loading, setLoading] = useState(false);
  const [roomInfo, setRoomInfo] = useState<any>(null);

  useEffect(() => {
    const fetchData = async () => {
      setLoading(true);
      try {
        const res = await axios.post(`${BASE_URL}/room/get_info`, {
          room_name: id,
        },
        {
          headers: {
            'Content-Type': 'application/json',
          },
        });
        setRoomInfo(res.data);
      } catch (err: any) {
        console.log(err);
        if (err.message) {
          toast.error(err.message);
        }
      } finally {
        setLoading(false);
      }
    };

    if (id) {
      fetchData();
    }
  }, [id]);

  return (
    <div className="h-dvh relative">
      <div className="size-full brightness-[.25] transition duration-300">
        <img className="object-cover size-full" src="/waiting.jpg" alt="waiting" />
      </div>
      <div className="fixed top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-12 items-center">
        {loading ? (
          <div className="text-5xl bg-white/90 text-black/80 rounded-xl px-4 py-2 font-bold text-center">
            Loading...
          </div>
        ) : (
          roomInfo && (
            <>
              <div className="text-5xl bg-white/90 text-black/80 rounded-xl px-4 py-2 font-bold text-center">
                <span>Waiting room: 
                  <span className="text-slate-700/80"> {roomInfo.room_name}</span>
                </span>
              </div>
              <div className="flex gap-20">
                <div className="text-3xl text-white/80 rounded-xl px-4 py-2 font-bold text-center">
                  <span>Host: {roomInfo.host}</span>
                </div>
                <div className="text-3xl text-white/80 rounded-xl px-4 py-2 font-bold text-center">
                  <span>Players: {roomInfo.users.length}/{roomInfo.capacity}</span>
                </div>
              </div>
              <div className="flex flex-wrap gap-8 justify-around">
              {roomInfo.users.map((user: any)=>(
                <div className="text-3xl bg-white/90 text-black/80 rounded-xl px-4 py-2 font-bold text-center">
                  <span>{user.username}</span>
                </div>
              ))}
              </div>
            </>
          )
        )}
      </div>
    </div>
  )
}