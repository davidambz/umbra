import type { ButtonHTMLAttributes } from "react";
import styles from "./Button.module.css";

type Variant = "primary" | "secondary" | "ghost" | "danger";

interface ButtonProps extends ButtonHTMLAttributes<HTMLButtonElement> {
  variant?: Variant;
}

export function Button({ variant = "secondary", className, ...rest }: ButtonProps) {
  return (
    <button className={[styles.button, styles[variant], className].filter(Boolean).join(" ")} {...rest} />
  );
}
