"use client";

import React, { useState, useEffect } from "react";
import {
    Activity,
    CreditCard,
    DollarSign, // Reusing existing icons or standard lucide ones
    Users,
    Download,
    Play,
    CheckCircle2,
    HardDrive
} from "lucide-react";

import { Button } from "@/components/ui/button";
import {
    Card,
    CardContent,
    CardDescription,
    CardHeader,
    CardTitle,
} from "@/components/ui/card";
import {
    Tabs,
    TabsContent,
    TabsTrigger,
} from "@/components/ui/tabs";

// Mock types for better dev experience (matches api-server/app/models.py)
interface Episode {
    id: number;
    episode_name: string;
    task_type: string;
    success: boolean;
    duration_seconds: number;
}

export default function Dashboard() {
    const [episodes, setEpisodes] = useState<Episode[]>([]);
    const [loading, setLoading] = useState(true);
    const [selectedEpisode, setSelectedEpisode] = useState<Episode | null>(null); // For future modal use
    const [videoUrl, setVideoUrl] = useState<string | null>(null);

    useEffect(() => {
        fetch("/api/episodes")
            .then(res => res.json())
            .then(data => {
                setEpisodes(data);
                setLoading(false);
            })
            .catch(err => {
                console.error("Failed to fetch episodes", err);
                setLoading(false);
            });
    }, []);

    const handleExport = async () => {
        const ids = episodes.filter(e => e.success).map(e => e.id);
        if (ids.length === 0) return;

        try {
            const res = await fetch("/api/datasets/export", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ episode_ids: ids, format: "lerobot_v3" })
            });
            const data = await res.json();
            alert(`Export Request Accepted: ${data.dataset_name}`);
        } catch (err) {
            console.error("Export failed", err);
            alert("Export failed. Check console.");
        }
    };

    return (
        <div className="flex-1 space-y-4">
            <div className="flex items-center justify-between space-y-2">
                <h2 className="text-3xl font-bold tracking-tight">Dashboard</h2>
                <div className="flex items-center space-x-2">
                    <Button onClick={handleExport} disabled={episodes.length === 0}>
                        <Download className="mr-2 h-4 w-4" />
                        Export to LeRobot
                    </Button>
                </div>
            </div>

            <Tabs className="space-y-4">
                <div className="flex items-center space-x-2 bg-muted p-1 rounded-md w-fit">
                    <TabsTrigger active>Overview</TabsTrigger>
                    <TabsTrigger>Analytics</TabsTrigger>
                    <TabsTrigger>Reports</TabsTrigger>
                </div>

                <TabsContent className="space-y-4">
                    <div className="grid gap-4 md:grid-cols-2 lg:grid-cols-4">
                        <Card>
                            <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
                                <CardTitle className="text-sm font-medium">
                                    Total Episodes
                                </CardTitle>
                                <Play className="h-4 w-4 text-muted-foreground" />
                            </CardHeader>
                            <CardContent>
                                <div className="text-2xl font-bold">{episodes.length}</div>
                                <p className="text-xs text-muted-foreground">
                                    +20.1% from last hour
                                </p>
                            </CardContent>
                        </Card>
                        <Card>
                            <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
                                <CardTitle className="text-sm font-medium">
                                    Success Rate
                                </CardTitle>
                                <CheckCircle2 className="h-4 w-4 text-muted-foreground" />
                            </CardHeader>
                            <CardContent>
                                <div className="text-2xl font-bold">100%</div>
                                <p className="text-xs text-muted-foreground">
                                    Validated data points
                                </p>
                            </CardContent>
                        </Card>
                        <Card>
                            <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
                                <CardTitle className="text-sm font-medium">Storage Used</CardTitle>
                                <HardDrive className="h-4 w-4 text-muted-foreground" />
                            </CardHeader>
                            <CardContent>
                                <div className="text-2xl font-bold">92%</div>
                                <p className="text-xs text-muted-foreground">
                                    +1.2GB new data
                                </p>
                            </CardContent>
                        </Card>
                        <Card>
                            <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
                                <CardTitle className="text-sm font-medium">
                                    Active Nodes
                                </CardTitle>
                                <Activity className="h-4 w-4 text-muted-foreground" />
                            </CardHeader>
                            <CardContent>
                                <div className="text-2xl font-bold">2</div>
                                <p className="text-xs text-muted-foreground">
                                    +1 since last hour
                                </p>
                            </CardContent>
                        </Card>
                    </div>

                    <div className="grid gap-4 md:grid-cols-2 lg:grid-cols-7">
                        <Card className="col-span-4">
                            <CardHeader>
                                <CardTitle>Recent Episodes</CardTitle>
                                <CardDescription>
                                    Latest data collected from edge workcells.
                                </CardDescription>
                            </CardHeader>
                            <CardContent>
                                <div className="relative w-full overflow-auto">
                                    <table className="w-full caption-bottom text-sm text-left">
                                        <thead className="[&_tr]:border-b">
                                            <tr className="border-b transition-colors hover:bg-muted/50 data-[state=selected]:bg-muted">
                                                <th className="h-12 px-4 align-middle font-medium text-muted-foreground">Environment</th>
                                                <th className="h-12 px-4 align-middle font-medium text-muted-foreground">Status</th>
                                                <th className="h-12 px-4 align-middle font-medium text-muted-foreground">Task</th>
                                                <th className="h-12 px-4 align-middle font-medium text-muted-foreground text-right">Duration</th>
                                            </tr>
                                        </thead>
                                        <tbody className="[&_tr:last-child]:border-0">
                                            {loading ? (
                                                <tr><td colSpan={4} className="p-4 text-center">Loading...</td></tr>
                                            ) : episodes.length === 0 ? (
                                                <tr><td colSpan={4} className="p-4 text-center text-muted-foreground">No episodes found</td></tr>
                                            ) : episodes.slice(0, 5).map((ep) => (
                                                <tr key={ep.id} className="border-b transition-colors hover:bg-muted/50 data-[state=selected]:bg-muted">
                                                    <td className="p-4 align-middle font-medium">{ep.episode_name}</td>
                                                    <td className="p-4 align-middle">
                                                        <div className={`inline-flex items-center rounded-full border px-2.5 py-0.5 text-xs font-semibold transition-colors focus:outline-none focus:ring-2 focus:ring-ring focus:ring-offset-2 ${ep.success ? 'border-transparent bg-green-500/10 text-green-700' : 'border-transparent bg-red-500/10 text-red-700'}`}>
                                                            {ep.success ? 'Success' : 'Failed'}
                                                        </div>
                                                    </td>
                                                    <td className="p-4 align-middle">{ep.task_type}</td>
                                                    <td className="p-4 align-middle text-right">{Math.round(ep.duration_seconds)}s</td>
                                                </tr>
                                            ))}
                                        </tbody>
                                    </table>
                                </div>
                            </CardContent>
                        </Card>
                        <Card className="col-span-3">
                            <CardHeader>
                                <CardTitle>Export Status</CardTitle>
                                <CardDescription>
                                    ML-Ready datasets generated today.
                                </CardDescription>
                            </CardHeader>
                            <CardContent>
                                <div className="space-y-8">
                                    <div className="flex items-center">
                                        <div className="h-9 w-9 rounded-full bg-blue-100 flex items-center justify-center text-blue-600 font-bold border border-blue-200">
                                            LR
                                        </div>
                                        <div className="ml-4 space-y-1">
                                            <p className="text-sm font-medium leading-none">lerobot_dataset_v3</p>
                                            <p className="text-sm text-muted-foreground">
                                                Processing • 12.5GB
                                            </p>
                                        </div>
                                        <div className="ml-auto font-medium">Just now</div>
                                    </div>
                                    {/* Placeholder items */}
                                    <div className="flex items-center">
                                        <div className="h-9 w-9 rounded-full bg-green-100 flex items-center justify-center text-green-600 font-bold border border-green-200">
                                            HF
                                        </div>
                                        <div className="ml-4 space-y-1">
                                            <p className="text-sm font-medium leading-none">huggingface_upload</p>
                                            <p className="text-sm text-muted-foreground">
                                                Completed • 8.2GB
                                            </p>
                                        </div>
                                        <div className="ml-auto font-medium">2h ago</div>
                                    </div>
                                </div>
                            </CardContent>
                        </Card>
                    </div>
                </TabsContent>
            </Tabs>
        </div>
    );
}
