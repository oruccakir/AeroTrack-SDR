interface CardProps {
  title?: string;
  children: React.ReactNode;
  className?: string;
}

export function Card({ title, children, className = "" }: CardProps) {
  return (
    <div
      className={`flex h-full flex-col rounded-lg border border-zinc-800 bg-zinc-900/80 backdrop-blur ${className}`}
    >
      {title && (
        <div className="shrink-0 border-b border-zinc-800 px-4 py-2">
          <h3 className="text-sm font-medium text-zinc-300">{title}</h3>
        </div>
      )}
      <div className="relative min-h-0 flex-1 p-2">{children}</div>
    </div>
  );
}
