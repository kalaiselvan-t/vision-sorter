import { LayoutDashboard, Database, Settings, Cuboid, Activity } from "lucide-react";
import { cn } from "@/lib/utils";
import Link from "next/link";
import { Button } from "@/components/ui/button";

export function Sidebar() {
    return (
        <div className="w-64 border-r bg-card min-h-screen flex flex-col">
            <div className="p-6 border-b flex items-center gap-2">
                <Cuboid className="w-6 h-6 text-primary" />
                <span className="font-bold text-lg tracking-tight">Focus Sorter</span>
            </div>

            <div className="flex-1 py-6 px-3 space-y-1">
                <h3 className="px-4 text-xs font-semibold text-muted-foreground uppercase tracking-wider mb-2">Platform</h3>

                <Link href="/">
                    <Button variant="secondary" className="w-full justify-start gap-2">
                        <LayoutDashboard className="w-4 h-4" />
                        Dashboard
                    </Button>
                </Link>

                <Button variant="ghost" className="w-full justify-start gap-2 text-muted-foreground">
                    <Database className="w-4 h-4" />
                    Data Lake (MinIO)
                </Button>

                <h3 className="px-4 text-xs font-semibold text-muted-foreground uppercase tracking-wider mb-2 mt-6">Settings</h3>

                <Button variant="ghost" className="w-full justify-start gap-2 text-muted-foreground">
                    <Activity className="w-4 h-4" />
                    Status
                </Button>

                <Button variant="ghost" className="w-full justify-start gap-2 text-muted-foreground">
                    <Settings className="w-4 h-4" />
                    Configuration
                </Button>
            </div>

            <div className="p-4 border-t">
                <div className="bg-muted/50 rounded-lg p-3 text-xs text-muted-foreground">
                    <div className="font-medium text-foreground mb-1">Workcell_01</div>
                    <div className="flex items-center gap-2">
                        <div className="w-2 h-2 rounded-full bg-green-500"></div>
                        Online
                    </div>
                </div>
            </div>
        </div>
    );
}
