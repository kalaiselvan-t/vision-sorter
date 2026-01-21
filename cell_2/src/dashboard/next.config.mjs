/** @type {import('next').NextConfig} */
const nextConfig = {
    output: "standalone",
    async rewrites() {
        return [
            {
                source: '/api/:path*',
                destination: `${process.env.API_URL || 'http://localhost:8000'}/:path*`, // Proxy to FastAPI
            },
        ]
    },
};

export default nextConfig;
