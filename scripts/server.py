import http.server
import socketserver

PORT = 8000


class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/':
            # if you rename the html change here the name
            self.path = '/index.html'
        return super().do_GET()

    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cache-Control', 'no-cache, no-store, must-revalidate')
        super().end_headers()


class ThreadingHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True


with ThreadingHTTPServer(("", PORT), Handler) as httpd:
    print(f"server on : http://localhost:{PORT}")
    httpd.serve_forever()
