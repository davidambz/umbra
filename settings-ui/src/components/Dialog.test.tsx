import { describe, expect, it, vi } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { useState } from "react";
import { Dialog } from "./Dialog";

// Mirrors how AddWallpaperDialog wraps Dialog: an onClose defined inline
// in the parent, recreated on every render — which happens on every
// keystroke into an owned text field.
function HostWithChangingOnClose() {
  const [value, setValue] = useState("");
  return (
    <Dialog title="Test dialog" onClose={() => {}}>
      <input aria-label="field" value={value} onChange={(event) => setValue(event.target.value)} />
    </Dialog>
  );
}

describe("Dialog", () => {
  it("does not steal focus from a child input when onClose's identity changes across renders", async () => {
    const user = userEvent.setup();
    render(<HostWithChangingOnClose />);

    const input = screen.getByLabelText("field");
    input.focus();
    expect(document.activeElement).toBe(input);

    await user.type(input, "hello");

    expect(document.activeElement).toBe(input);
    expect(input).toHaveValue("hello");
  });

  it("still calls the latest onClose on Escape", async () => {
    const user = userEvent.setup();
    const onClose = vi.fn();
    render(
      <Dialog title="Test dialog" onClose={onClose}>
        <p>content</p>
      </Dialog>,
    );

    await user.keyboard("{Escape}");
    expect(onClose).toHaveBeenCalledTimes(1);
  });
});
