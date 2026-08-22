import styles from "./Toggle.module.css";

interface ToggleProps {
  label: string;
  description?: string;
  checked: boolean;
  onChange: (checked: boolean) => void;
}

export function Toggle({ label, description, checked, onChange }: ToggleProps) {
  return (
    <label className={styles.row}>
      <span className={styles.text}>
        <span className={styles.label}>{label}</span>
        {description && <span className={styles.description}>{description}</span>}
      </span>
      <span className={styles.switchWrapper}>
        <input
          type="checkbox"
          checked={checked}
          onChange={(event) => onChange(event.target.checked)}
          className={styles.input}
        />
        <span className={styles.track}>
          <span className={styles.thumb} />
        </span>
      </span>
    </label>
  );
}
