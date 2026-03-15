import { InputHTMLAttributes } from "react";

interface InputProps extends InputHTMLAttributes<HTMLInputElement> {
  label: string;
  unit?: string;
}

export function Input({ label, unit, className = "", ...props }: InputProps) {
  return (
    <div className="flex flex-col gap-1">
      <label className="text-xs text-zinc-500">{label}</label>
      <div className="flex items-center gap-1">
        <input
          className={`w-full rounded border border-zinc-700 bg-zinc-800 px-2 py-1 text-sm text-zinc-200 outline-none focus:border-emerald-500 ${className}`}
          {...props}
        />
        {unit && <span className="text-xs text-zinc-500 whitespace-nowrap">{unit}</span>}
      </div>
    </div>
  );
}
