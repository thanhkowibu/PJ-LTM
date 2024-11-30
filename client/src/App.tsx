import { useState } from 'react'
import './App.css'
import { BrowserRouter, Route, Routes } from "react-router-dom";
import routes from './routes/routes';


function App() {
  const [count, setCount] = useState(0)

  return (
    <>
      <BrowserRouter>
          <Routes>
            <Route path="/">
              {routes.map((route, index) =>
                  <Route
                    path={route.path}
                    key={index}
                    element={route.element}
                  />
              )}
            </Route>
          </Routes>
      </BrowserRouter>
    </>
  )
}

export default App
