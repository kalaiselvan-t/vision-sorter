"use client";

import React, { useState, useEffect, useCallback } from "react";
import {
    Activity,
    CreditCard,
    DollarSign,
    Users,
    Download,
    Play,
    CheckCircle2,
    HardDrive,
    Filter,
    Database,
    Eye,
    CheckSquare,
    Square,
    X,
    ChevronRight,
    Search,
    Calendar,
    RefreshCcw
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
    TabsList
} from "@/components/ui/tabs";

// Mock types for better dev experience (matches api-server/app/models.py)
interface Episode {
    id: number;
    episode_name: string;
    task_type: string;
    success: boolean;
    duration_seconds: number;
}

interface Dataset {
    name: string;
    path: string;
    last_modified: string;
}

export default function Dashboard() {
    const [episodes, setEpisodes] = useState<Episode[]>([]);
    const [datasets, setDatasets] = useState<Dataset[]>([]);
    const [loading, setLoading] = useState(true);
    const [datasetsLoading, setDatasetsLoading] = useState(false);
    const [selectedEpisode, setSelectedEpisode] = useState<Episode | null>(null);
    const [videoUrl, setVideoUrl] = useState<string | null>(null);

    // Selection state
    const [selectedIds, setSelectedIds] = useState<Set<number>>(new Set());

    // Tabs state
    const [activeTab, setActiveTab] = useState<string>("overview");

    // Filtering state
    const [filterTaskType, setFilterTaskType] = useState<string>("");
    const [filterSuccess, setFilterSuccess] = useState<string>("all"); // "all", "true", "false"

    const fetchEpisodes = useCallback(() => {
        setLoading(true);
        let url = "/api/episodes?limit=100";
        if (filterTaskType) url += `&task_type=${filterTaskType}`;
        if (filterSuccess !== "all") url += `&success=${filterSuccess}`;

        fetch(url)
            .then(res => res.json())
            .then(data => {
                setEpisodes(data);
                setLoading(false);
            })
            .catch(err => {
                console.error("Failed to fetch episodes", err);
                setLoading(false);
            });
    }, [filterTaskType, filterSuccess]);

    const fetchDatasets = useCallback(() => {
        setDatasetsLoading(true);
        fetch("/api/datasets")
            .then(res => res.json())
            .then(data => {
                setDatasets(data);
                setDatasetsLoading(false);
            })
            .catch(err => {
                console.error("Failed to fetch datasets", err);
                setDatasetsLoading(false);
            });
    }, []);

    useEffect(() => {
        fetchEpisodes();
        fetchDatasets();
    }, [fetchEpisodes, fetchDatasets]);

    const handleExport = async () => {
        const ids = Array.from(selectedIds).map(id => Number(id));
        console.log("Exporting episode IDs:", ids);

        if (ids.length === 0) {
            alert("Please select at least one episode to export.");
            return;
        }

        try {
            const res = await fetch("/api/datasets/export", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ episode_ids: ids, format: "lerobot_v3" })
            });

            if (!res.ok) {
                const errorData = await res.json();
                throw new Error(errorData.detail || "Export failed");
            }

            const data = await res.json();
            alert(`Export Request Accepted: ${data.dataset_name}. Generation started in background.`);
            fetchDatasets(); // Refresh list
        } catch (err: any) {
            console.error("Export failed", err);
            alert(`Export failed: ${err.message}`);
        }
    };

    const toggleSelection = (id: number) => {
        setSelectedIds((prev: Set<number>) => {
            const next = new Set(prev);
            if (next.has(id)) next.delete(id);
            else next.add(id);
            return next;
        });
    };

    const selectAll = () => {
        if (selectedIds.size === episodes.length) {
            setSelectedIds(new Set());
        } else {
            setSelectedIds(new Set(episodes.map(e => e.id)));
        }
    };

    const openVideo = async (episode: Episode) => {
        setSelectedEpisode(episode);
        setVideoUrl(null);
        try {
            const res = await fetch(`/api/episodes/${episode.id}/video_url`);
            const data = await res.json();
            setVideoUrl(data.url);
        } catch (err) {
            console.error("Failed to fetch video URL", err);
        }
    };

    return (
        <div className="flex-1 space-y-4 p-8 pt-6 bg-slate-50/50 min-h-screen">
            <div className="flex items-center justify-between space-y-2">
                <div>
                    <h2 className="text-3xl font-bold tracking-tight text-slate-900">Intrinsic Data Hub</h2>
                    <p className="text-slate-500">Manage and export robot episodes for model training.</p>
                </div>
                <div className="flex items-center space-x-2">
                    <Button
                        onClick={handleExport}
                        disabled={selectedIds.size === 0}
                        className="bg-blue-600 hover:bg-blue-700 text-white shadow-lg transition-all"
                    >
                        <Download className="mr-2 h-4 w-4" />
                        Export {selectedIds.size > 0 ? `(${selectedIds.size})` : ""} to LeRobot
                    </Button>
                </div>
            </div>

            <Tabs className="space-y-4">
                <TabsList className="bg-white border border-slate-200 p-1">
                    <TabsTrigger active={activeTab === 'overview'} onClick={() => setActiveTab('overview')}>Overview</TabsTrigger>
                    <TabsTrigger active={activeTab === 'datasets'} onClick={() => setActiveTab('datasets')}>Exported Datasets</TabsTrigger>
                    <TabsTrigger active={activeTab === 'analytics'} onClick={() => setActiveTab('analytics')}>System Health</TabsTrigger>
                </TabsList>

                <TabsContent value="overview" className={activeTab === 'overview' ? 'block space-y-4' : 'hidden'}>
                    {/* Stats Section */}
                    <div className="grid gap-4 md:grid-cols-2 lg:grid-cols-4">
                        <Card className="border-none shadow-sm bg-white hover:shadow-md transition-shadow">
                            <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
                                <CardTitle className="text-sm font-medium text-slate-600">Total Episodes</CardTitle>
                                <Play className="h-4 w-4 text-blue-500" />
                            </CardHeader>
                            <CardContent>
                                <div className="text-2xl font-bold">{episodes.length}</div>
                                <p className="text-xs text-slate-400 mt-1">Found in current filter</p>
                            </CardContent>
                        </Card>
                        <Card className="border-none shadow-sm bg-white hover:shadow-md transition-shadow">
                            <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
                                <CardTitle className="text-sm font-medium text-slate-600">Success Rate</CardTitle>
                                <CheckCircle2 className="h-4 w-4 text-green-500" />
                            </CardHeader>
                            <CardContent>
                                <div className="text-2xl font-bold">
                                    {episodes.length > 0 ? Math.round((episodes.filter(e => e.success).length / episodes.length) * 100) : 0}%
                                </div>
                                <p className="text-xs text-slate-400 mt-1">Filtered episodes</p>
                            </CardContent>
                        </Card>
                        <Card className="border-none shadow-sm bg-white hover:shadow-md transition-shadow">
                            <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
                                <CardTitle className="text-sm font-medium text-slate-600">Selected</CardTitle>
                                <CheckSquare className="h-4 w-4 text-purple-500" />
                            </CardHeader>
                            <CardContent>
                                <div className="text-2xl font-bold text-purple-600">{selectedIds.size}</div>
                                <p className="text-xs text-slate-400 mt-1">Ready for export</p>
                            </CardContent>
                        </Card>
                        <Card className="border-none shadow-sm bg-white hover:shadow-md transition-shadow">
                            <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
                                <CardTitle className="text-sm font-medium text-slate-600">Active Cells</CardTitle>
                                <Activity className="h-4 w-4 text-orange-500" />
                            </CardHeader>
                            <CardContent>
                                <div className="text-2xl font-bold">2</div>
                                <p className="text-xs text-slate-400 mt-1">Edge nodes uploading</p>
                            </CardContent>
                        </Card>
                    </div>

                    <div className="flex flex-col lg:flex-row gap-4">
                        {/* Filters Sidebar/Section */}
                        <Card className="lg:w-64 flex-shrink-0 border-none shadow-sm bg-white h-fit sticky top-4">
                            <CardHeader className="pb-3 border-b border-slate-100">
                                <CardTitle className="text-sm font-semibold flex items-center">
                                    <Filter className="mr-2 h-4 w-4" /> Visual Filter
                                </CardTitle>
                            </CardHeader>
                            <CardContent className="pt-4 space-y-4">
                                <div className="space-y-2">
                                    <label className="text-xs font-medium text-slate-500 uppercase">Task Type</label>
                                    <select
                                        className="w-full text-sm border-slate-200 rounded-md focus:ring-blue-500 focus:border-blue-500 p-2 border"
                                        value={filterTaskType}
                                        onChange={(e: React.ChangeEvent<HTMLSelectElement>) => setFilterTaskType(e.target.value)}
                                    >
                                        <option value="">All Tasks</option>
                                        <option value="cell_1">Cell 1 (Pick & Place)</option>
                                        <option value="cell_2">Cell 2 (Advanced Sorting)</option>
                                    </select>
                                </div>
                                <div className="space-y-2">
                                    <label className="text-xs font-medium text-slate-500 uppercase">Status</label>
                                    <div className="flex flex-col space-y-1">
                                        <button
                                            onClick={() => setFilterSuccess("all")}
                                            className={`text-left px-2 py-1.5 text-sm rounded-md transition-colors ${filterSuccess === 'all' ? 'bg-blue-50 text-blue-700 font-medium' : 'hover:bg-slate-50 text-slate-600'}`}
                                        >
                                            All Results
                                        </button>
                                        <button
                                            onClick={() => setFilterSuccess("true")}
                                            className={`text-left px-2 py-1.5 text-sm rounded-md transition-colors ${filterSuccess === 'true' ? 'bg-green-50 text-green-700 font-medium' : 'hover:bg-slate-50 text-slate-600'}`}
                                        >
                                            Success Only
                                        </button>
                                        <button
                                            onClick={() => setFilterSuccess("false")}
                                            className={`text-left px-2 py-1.5 text-sm rounded-md transition-colors ${filterSuccess === 'false' ? 'bg-red-50 text-red-700 font-medium' : 'hover:bg-slate-50 text-slate-600'}`}
                                        >
                                            Failures Only
                                        </button>
                                    </div>
                                </div>
                                <Button
                                    variant="outline"
                                    size="sm"
                                    className="w-full mt-4 flex items-center justify-center"
                                    onClick={() => {
                                        setFilterTaskType("");
                                        setFilterSuccess("all");
                                    }}
                                >
                                    <RefreshCcw className="mr-2 h-3 w-3" /> Reset Filters
                                </Button>
                            </CardContent>
                        </Card>

                        {/* Episodes Table Content */}
                        <div className="flex-1 space-y-4">
                            <Card className="border-none shadow-sm bg-white overflow-hidden">
                                <CardHeader className="flex flex-row items-center justify-between border-b border-slate-100">
                                    <div>
                                        <CardTitle>Recent Episodes</CardTitle>
                                        <CardDescription>Select episodes to bundle into your ML dataset.</CardDescription>
                                    </div>
                                    <div className="flex items-center space-x-2">
                                        <Button variant="outline" size="sm" onClick={selectAll}>
                                            {selectedIds.size === episodes.length ? "Deselect All" : "Select All"}
                                        </Button>
                                    </div>
                                </CardHeader>
                                <CardContent className="p-0">
                                    <div className="relative w-full overflow-auto">
                                        <table className="w-full caption-bottom text-sm text-left border-collapse">
                                            <thead className="bg-slate-50/50">
                                                <tr className="border-b border-slate-100 transition-colors">
                                                    <th className="h-12 w-12 px-4 align-middle font-medium text-slate-500">
                                                        <input
                                                            type="checkbox"
                                                            checked={episodes.length > 0 && selectedIds.size === episodes.length}
                                                            onChange={selectAll}
                                                            className="rounded border-slate-300 text-blue-600 focus:ring-blue-500"
                                                        />
                                                    </th>
                                                    <th className="h-12 px-4 align-middle font-medium text-slate-500">Episode Name</th>
                                                    <th className="h-12 px-4 align-middle font-medium text-slate-500">Status</th>
                                                    <th className="h-12 px-4 align-middle font-medium text-slate-500">Task</th>
                                                    <th className="h-12 px-4 align-middle font-medium text-slate-500 text-right">Duration</th>
                                                    <th className="h-12 px-4 align-middle font-medium text-slate-500 text-right pr-6">Action</th>
                                                </tr>
                                            </thead>
                                            <tbody className="divide-y divide-slate-100">
                                                {loading ? (
                                                    <tr><td colSpan={6} className="p-12 text-center text-slate-400">
                                                        <div className="flex flex-col items-center">
                                                            <RefreshCcw className="h-8 w-8 animate-spin mb-2" />
                                                            Loading episodes...
                                                        </div>
                                                    </td></tr>
                                                ) : episodes.length === 0 ? (
                                                    <tr><td colSpan={6} className="p-12 text-center text-slate-400 italic">No episodes match your current filters.</td></tr>
                                                ) : episodes.map((ep) => (
                                                    <tr
                                                        key={ep.id}
                                                        className={`transition-colors hover:bg-slate-50/80 ${selectedIds.has(ep.id) ? 'bg-blue-50/30' : ''}`}
                                                    >
                                                        <td className="p-4 align-middle">
                                                            <input
                                                                type="checkbox"
                                                                checked={selectedIds.has(ep.id)}
                                                                onChange={() => toggleSelection(ep.id)}
                                                                className="rounded border-slate-300 text-blue-600 focus:ring-blue-500"
                                                            />
                                                        </td>
                                                        <td className="p-4 align-middle font-medium text-slate-900 group">
                                                            <div className="flex items-center">
                                                                {ep.episode_name}
                                                                <Eye
                                                                    className="ml-2 h-3 w-3 text-slate-300 group-hover:text-blue-500 cursor-pointer"
                                                                    onClick={(e) => {
                                                                        e.stopPropagation();
                                                                        openVideo(ep);
                                                                    }}
                                                                />
                                                            </div>
                                                        </td>
                                                        <td className="p-4 align-middle">
                                                            <div className={`inline-flex items-center rounded-full px-2.5 py-0.5 text-xs font-semibold ${ep.success ? 'bg-green-100 text-green-700 border border-green-200' : 'bg-red-100 text-red-700 border border-red-200'}`}>
                                                                {ep.success ? 'Success' : 'Failed'}
                                                            </div>
                                                        </td>
                                                        <td className="p-4 align-middle text-slate-600">{ep.task_type}</td>
                                                        <td className="p-4 align-middle text-right text-slate-600">{Math.round(ep.duration_seconds)}s</td>
                                                        <td className="p-4 align-middle text-right pr-6">
                                                            <Button variant="ghost" size="sm" className="h-8 text-blue-600 hover:text-blue-700 hover:bg-blue-50" onClick={() => openVideo(ep)}>
                                                                View Clip
                                                            </Button>
                                                        </td>
                                                    </tr>
                                                ))}
                                            </tbody>
                                        </table>
                                    </div>
                                </CardContent>
                            </Card>
                        </div>
                    </div>
                </TabsContent>

                <TabsContent value="datasets" className={activeTab === 'datasets' ? 'block space-y-4' : 'hidden'}>
                    <div className="grid gap-4 md:grid-cols-2 lg:grid-cols-3">
                        {datasetsLoading ? (
                            <Card className="col-span-full border-dashed border-2 border-slate-200 bg-transparent flex items-center justify-center p-12">
                                <div className="text-center text-slate-400">
                                    <RefreshCcw className="h-8 w-8 animate-spin mx-auto mb-2" />
                                    Scanning MinIO for datasets...
                                </div>
                            </Card>
                        ) : datasets.length === 0 ? (
                            <Card className="col-span-full border-dashed border-2 border-slate-200 bg-transparent flex items-center justify-center p-12">
                                <div className="text-center text-slate-400">
                                    <Database className="h-12 w-12 mx-auto mb-4 opacity-20" />
                                    <p>No exported datasets found in MinIO.</p>
                                    <p className="text-sm mt-1">Select episodes on the Overview tab to create one.</p>
                                </div>
                            </Card>
                        ) : datasets.map((ds: Dataset) => (
                            <Card key={ds.name} className="border-none shadow-sm bg-white hover:shadow-md transition-shadow group">
                                <CardHeader>
                                    <div className="flex justify-between items-start">
                                        <div className="h-10 w-10 rounded-lg bg-blue-100 flex items-center justify-center text-blue-600 font-bold border border-blue-200">
                                            LR
                                        </div>
                                        <div className={`px-2 py-1 rounded text-[10px] font-bold uppercase ${ds.name.includes('test') ? 'bg-amber-100 text-amber-700' : 'bg-indigo-100 text-indigo-700'}`}>
                                            LeRobot V3
                                        </div>
                                    </div>
                                    <CardTitle className="mt-4 text-slate-900">{ds.name}</CardTitle>
                                    <CardDescription className="truncate font-mono text-[11px]">{ds.path}</CardDescription>
                                </CardHeader>
                                <CardContent>
                                    <div className="flex justify-between items-center text-xs text-slate-400">
                                        <span>Modified: {new Date(ds.last_modified).toLocaleDateString()}</span>
                                        <Button variant="ghost" size="sm" className="h-8 px-2 group-hover:text-blue-600 group-hover:bg-blue-50">
                                            Manage <ChevronRight className="ml-1 h-3 w-3" />
                                        </Button>
                                    </div>
                                </CardContent>
                            </Card>
                        ))}
                    </div>
                </TabsContent>

                <TabsContent value="analytics" className={activeTab === 'analytics' ? 'block space-y-4' : 'hidden'}>
                    <Card className="border-none shadow-sm bg-white">
                        <CardHeader>
                            <CardTitle>Workcell Performance</CardTitle>
                            <CardDescription>Live telemetry from active edge nodes.</CardDescription>
                        </CardHeader>
                        <CardContent className="h-64 flex items-center justify-center border-t border-slate-50">
                            <div className="text-center text-slate-300">
                                <Activity className="h-16 w-16 mx-auto mb-4 opacity-10" />
                                <p>Analytics visualization would render here.</p>
                                <p className="text-sm">Connecting to system_metrics hypertable...</p>
                            </div>
                        </CardContent>
                    </Card>
                </TabsContent>
            </Tabs>

            {/* Video Player Modal/Overlay */}
            {selectedEpisode && (
                <div className="fixed inset-0 bg-slate-900/80 backdrop-blur-sm z-50 flex items-center justify-center p-4">
                    <Card className="w-full max-w-4xl bg-white border-none shadow-2xl overflow-hidden">
                        <CardHeader className="flex flex-row items-center justify-between pb-4 border-b">
                            <div>
                                <CardTitle className="text-xl">Preview: {selectedEpisode.episode_name}</CardTitle>
                                <CardDescription>Task: {selectedEpisode.task_type} • Status: {selectedEpisode.success ? 'Success' : 'Failed'}</CardDescription>
                            </div>
                            <Button variant="ghost" size="icon" onClick={() => setSelectedEpisode(null)} className="rounded-full hover:bg-slate-100">
                                <X className="h-5 w-5" />
                            </Button>
                        </CardHeader>
                        <CardContent className="p-0 bg-black aspect-video flex items-center justify-center relative">
                            {videoUrl ? (
                                <video
                                    src={videoUrl}
                                    controls
                                    autoPlay
                                    className="w-full h-full"
                                />
                            ) : (
                                <div className="text-white flex flex-col items-center">
                                    <RefreshCcw className="h-8 w-8 animate-spin mb-4" />
                                    Fetching video stream from MinIO...
                                </div>
                            )}
                        </CardContent>
                    </Card>
                </div>
            )}
        </div>
    );
}
