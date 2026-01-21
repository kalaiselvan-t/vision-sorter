import { Search, Bell, UserCircle } from "lucide-react";
import { Button } from "@/components/ui/button";

export function Header() {
    return (
        <header className="h-16 border-b bg-background flex items-center justify-between px-6 sticky top-0 z-10">
            <div className="flex items-center gap-4 text-sm text-muted-foreground">
                <span>Intrinsic</span>
                <span className="text-muted-foreground/30">/</span>
                <span className="text-foreground font-medium">Data Hub</span>
            </div>

            <div className="flex items-center gap-3">
                <div className="relative">
                    <Search className="absolute left-2.5 top-1/2 -translate-y-1/2 w-4 h-4 text-muted-foreground" />
                    <input
                        type="text"
                        placeholder="Search resources..."
                        className="h-9 w-64 rounded-md border border-input bg-background pl-9 pr-3 text-sm ring-offset-background placeholder:text-muted-foreground focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring"
                    />
                </div>

                <Button variant="ghost" size="icon">
                    <Bell className="w-4 h-4" />
                </Button>

                <Button variant="ghost" size="icon">
                    <UserCircle className="w-5 h-5" />
                </Button>
            </div>
        </header>
    );
}
