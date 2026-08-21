import type { LibraryItem } from "../types";
import { WallpaperCard } from "./WallpaperCard";
import { Button } from "./Button";
import styles from "./WallpaperLibrary.module.css";

interface WallpaperLibraryProps {
  library: LibraryItem[];
  onAdd: () => void;
  onRename: (id: string, newTitle: string) => void;
  onRemove: (id: string) => void;
}

export function WallpaperLibrary({ library, onAdd, onRename, onRemove }: WallpaperLibraryProps) {
  return (
    <section>
      <div className={styles.header}>
        <h2 className={styles.heading}>Library</h2>
        <Button variant="primary" onClick={onAdd}>
          + Add wallpaper
        </Button>
      </div>

      {library.length === 0 ? (
        <p className={styles.empty}>
          Nothing imported yet — add a video, image, or web project to assign it to a monitor.
        </p>
      ) : (
        <div className={styles.grid}>
          {library.map((item) => (
            <WallpaperCard
              key={item.id}
              item={item}
              onRename={(newTitle) => onRename(item.id, newTitle)}
              onRemove={() => onRemove(item.id)}
            />
          ))}
        </div>
      )}
    </section>
  );
}
