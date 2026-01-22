import type { Metadata } from "next";
import { Inter } from "next/font/google"; // Font logic remains
import "./globals.css";
import { Sidebar } from "@/components/layout/Sidebar";
import { Header } from "@/components/layout/Header";

const inter = Inter({ subsets: ["latin"] });

export const metadata: Metadata = {
    title: "Intrinsic Data Hub",
    description: "Next-gen telemetry and dataset management",
};

export default function RootLayout({
    children,
}: Readonly<{
    children: React.ReactNode;
}>) {
    return (
        <html lang="en">
            <body className={`${inter.className} min-h-screen bg-background text-foreground`}>
                <div className="flex min-h-screen">
                    {/* Sidebar: Fixed width (defined in component) */}
                    <Sidebar />

                    {/* Main Content Area: Flex grow */}
                    <div className="flex-1 flex flex-col">
                        <Header />
                        <main className="flex-1 p-6 bg-muted/20 overflow-y-auto">
                            {children}
                        </main>
                    </div>
                </div>
            </body>
        </html>
    );
}
