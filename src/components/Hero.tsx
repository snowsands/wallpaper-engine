import { invoke } from "@tauri-apps/api/core";
import { useEffect, useState } from "react";
import { open } from "@tauri-apps/plugin-dialog";

import backgroundImg from "../assets/yottea96.jpg";

import "./Hero.css";

function Front() {
  async function pickWallpaper() {
    const file = await open({
      multiple: false,
      filters: [
        {
          name: "Videos",
          extensions: ["mp4", "webm", "mkv", "avi", "gif"],
        },
      ],
    });

    if (!file) return;

    await invoke("set_wallpaper", {
      fileName: file,
    });
  }

  return (
    <>
      <div className="hero">
        <div className="bg">
          <img src={backgroundImg}></img>
        </div>

        <div className="content">
          <div className="header">
            <h1>Wallpaper Engine</h1>
          </div>
          <div className="info">
            Can change your wallpaper, accepts mp4, webm, mkv, avi and gif
            formats
          </div>

          <div className="change-wp">
            <button onClick={pickWallpaper}>Choose Wallpaper</button>
          </div>
        </div>
      </div>
    </>
  );
}

export default Front;
