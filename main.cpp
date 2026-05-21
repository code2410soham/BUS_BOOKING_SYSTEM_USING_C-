#include "json.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// Windows-specific setup for console virtual terminal processing (colors)
#ifdef _WIN32
#ifdef __clang__
#define __rdtsc __rdtsc_dummy
#define _mm_getcsr _mm_getcsr_dummy
#define _mm_setcsr _mm_setcsr_dummy
#define _mm_sfence _mm_sfence_dummy
#define _mm_lfence _mm_lfence_dummy
#define _mm_mfence _mm_mfence_dummy
#define _mm_pause _mm_pause_dummy
#define _mm_clflush _mm_clflush_dummy
#define _m_prefetchw _m_prefetchw_dummy
#define _m_prefetch _m_prefetch_dummy
#endif
#include <conio.h>
#include <windows.h>
#include <shellapi.h>
#ifdef __clang__
#undef __rdtsc
#undef _mm_getcsr
#undef _mm_setcsr
#undef _mm_sfence
#undef _mm_lfence
#undef _mm_mfence
#undef _mm_pause
#undef _mm_clflush
#undef _m_prefetchw
#undef _m_prefetch
#endif
#else
#include <termios.h>
#include <unistd.h>
#endif

// Visual styling colors
namespace Color {
const std::string RESET = "\033[0m";
const std::string BOLD = "\033[1m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";
const std::string WHITE = "\033[37m";

const std::string BRIGHT_BLACK = "\033[90m";
const std::string LIGHT_CYAN = "\033[96m";
const std::string LIGHT_BLUE = "\033[94m";
const std::string BRIGHT_GREEN = "\033[92m";
const std::string BRIGHT_RED = "\033[91m";

// Backgrounds
const std::string BG_RED = "\033[41m";
const std::string BG_GREEN = "\033[42m";
const std::string BG_YELLOW = "\033[43m";
const std::string BG_BLUE = "\033[44m";
const std::string BG_GRAY = "\033[100m";
} // namespace Color

// Global cross-platform console color activation helper
void enableVirtualTerminal() {
#ifdef _WIN32
  // Set console input and output code pages to UTF-8 for proper Unicode
  // rendering
  SetConsoleOutputCP(65001);
  SetConsoleCP(65001);

  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE)
    return;
  DWORD dwMode = 0;
  if (!GetConsoleMode(hOut, &dwMode))
    return;
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);
#endif
}

#ifdef _WIN32
struct EnumData {
  HWND hwnd = NULL;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
  EnumData *data = (EnumData *)lParam;
  wchar_t title[256];
  if (GetWindowTextW(hwnd, title, 256) > 0) {
    std::wstring wTitle(title);
    if (wTitle.find(L"WhatsApp") != std::wstring::npos) {
      data->hwnd = hwnd;
      return FALSE; // stop enumeration
    }
  }
  return TRUE;
}

HWND findWhatsAppWindow() {
  EnumData data;
  EnumWindows(EnumWindowsProc, (LPARAM)&data);
  return data.hwnd;
}

void simulateKeyPress(BYTE vk) {
  keybd_event(vk, 0, 0, 0);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  keybd_event(vk, 0, KEYEVENTF_KEYUP, 0);
}

void simulateCtrlEnter() {
  keybd_event(VK_CONTROL, 0, 0, 0);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  keybd_event(VK_RETURN, 0, 0, 0);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
  keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
}

void simulateShiftTab() {
  keybd_event(VK_SHIFT, 0, 0, 0);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  keybd_event(VK_TAB, 0, 0, 0);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  keybd_event(VK_TAB, 0, KEYEVENTF_KEYUP, 0);
  keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
}

void runWhatsAppAutomation() {
  // Wait up to 10 seconds for WhatsApp window to appear (20 polls x 250ms)
  HWND hwnd = NULL;
  for (int i = 0; i < 40; ++i) {
    hwnd = findWhatsAppWindow();
    if (hwnd != NULL)
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }

  if (hwnd == NULL)
    return;

  // Wait 1.5s for WhatsApp window to paint the message input
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  // Attempt transmission — 4 sweeps with tighter inter-attempt gap
  for (int attempt = 0; attempt < 4; ++attempt) {
    // Bring WhatsApp to foreground
    ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 1. Try standard Enter (if "Enter to send" chats option is active)
    simulateKeyPress(VK_RETURN);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 2. Try Ctrl+Enter (standard UWP keyboard send shortcut)
    simulateCtrlEnter();

    // Shorter delay before next sweep
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
  }
}
#endif

// Cross-platform key fetcher for password masking
char getConsoleChar() {
#ifdef _WIN32
  return _getch();
#else
  char buf = 0;
  struct termios old = {0};
  if (tcgetattr(0, &old) < 0)
    perror("tcsetattr()");
  old.c_lflag &= ~ICANON;
  old.c_lflag &= ~ECHO;
  old.c_cc[VMIN] = 1;
  old.c_cc[VTIME] = 0;
  if (tcsetattr(0, TCSANOW, &old) < 0)
    perror("tcsetattr ~ICANON");
  if (read(0, &buf, 1) < 0)
    perror("read()");
  old.c_lflag |= ICANON;
  old.c_lflag |= ECHO;
  if (tcsetattr(0, TCSADRAIN, &old) < 0)
    perror("tcsetattr ICANON");
  return buf;
#endif
}

// -------------------------------------------------------------
// Data Structures
// -------------------------------------------------------------

struct Passenger {
  std::string name;
  int age;
  char gender; // M, F, O
};

struct User {
  std::string username;
  std::string passwordHash;
  std::string fullName;
  std::string phone;
  std::string role; // "ADMIN" or "CUSTOMER"
};

struct Bus {
  std::string busId;
  std::string busName;
  std::string
      busType; // "AC Seater", "Non-AC Seater", "AC Sleeper", "Non-AC Sleeper"
  std::string source;
  std::string destination;
  std::string depTime; // e.g. "08:00 AM"
  std::string arrTime; // e.g. "02:00 PM"
  double fare;
  int totalSeats;
};

struct Booking {
  std::string bookingId;
  std::string username;
  std::string busId;
  std::string date; // "YYYY-MM-DD"
  std::vector<int> seatNumbers;
  std::vector<Passenger> passengers;
  double farePaid;
  std::string status; // "BOOKED" or "CANCELLED"
  std::string timestamp;
};

struct RouteRequest {
  std::string requestId;
  std::string username;
  std::string source;
  std::string destination;
  std::string preferredTime; // e.g. "08:00 AM"
  std::string notes;
  std::string status;    // "PENDING" or "APPROVED" or "REJECTED"
  std::string timestamp;
};

// -------------------------------------------------------------
// Helper Functions
// -------------------------------------------------------------

void clearScreen() {
#ifdef _WIN32
  HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  DWORD count;
  DWORD cellCount;
  COORD homeCoords = {0, 0};
  if (hStdOut == INVALID_HANDLE_VALUE)
    return;
  if (!GetConsoleScreenBufferInfo(hStdOut, &csbi))
    return;
  cellCount = csbi.dwSize.X * csbi.dwSize.Y;
  if (!FillConsoleOutputCharacter(hStdOut, (TCHAR)' ', cellCount, homeCoords,
                                  &count))
    return;
  if (!FillConsoleOutputAttribute(hStdOut, csbi.wAttributes, cellCount,
                                  homeCoords, &count))
    return;
  SetConsoleCursorPosition(hStdOut, homeCoords);
#else
  std::system("clear");
#endif
}

std::string urlEncode(const std::string &value) {
  std::ostringstream escaped;
  escaped << std::hex;
  for (char c : value) {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped << c;
    } else {
      escaped << '%' << std::setw(2) << std::setfill('0')
              << ((int)(unsigned char)c);
    }
  }
  return escaped.str();
}

void openBrowser(const std::string &url) {
#ifdef _WIN32
  if (url.find("://") != std::string::npos) {
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
  } else {
    char absPath[MAX_PATH];
    if (_fullpath(absPath, url.c_str(), MAX_PATH) != NULL) {
      ShellExecuteA(NULL, "open", absPath, NULL, NULL, SW_SHOWNORMAL);
    } else {
      ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
  }
#else
  std::string cmd = "xdg-open \"" + url + "\" &";
  std::system(cmd.c_str());
#endif
}

void generateHtmlTicket(const Booking &booking, const Bus &bus) {
#ifdef _WIN32
  CreateDirectoryA("tickets", NULL);
#else
  std::system("mkdir -p tickets");
#endif

  std::string filename = "tickets/ticket_" + booking.bookingId + ".html";
  std::ofstream file(filename);
  if (!file.is_open())
    return;

  // Dynamically build QR Code verification payload
  std::stringstream qrData;
  qrData << "AEROLINE TRAVELS BOARDING PASS\n"
         << "Booking Ref: " << booking.bookingId << "\n"
         << "Status: " << booking.status << "\n"
         << "Bus: " << bus.busName << " (" << bus.busType << ")\n"
         << "Route: " << bus.source << " -> " << bus.destination << "\n"
         << "Date & Time: " << booking.date << " | " << bus.depTime << "\n"
         << "Seats: ";
  for (size_t i = 0; i < booking.seatNumbers.size(); ++i) {
    qrData << booking.seatNumbers[i]
           << (i + 1 < booking.seatNumbers.size() ? ", " : "");
  }
  qrData << "\nPassengers:\n";
  for (size_t i = 0; i < booking.passengers.size(); ++i) {
    qrData << "- " << booking.passengers[i].name << " ("
           << booking.passengers[i].age << " | " << booking.passengers[i].gender
           << ")\n";
  }
  qrData << "Fare Paid: Rs. " << booking.farePaid;
  std::string encodedQr = urlEncode(qrData.str());
  std::string qrUrl = "https://api.qrserver.com/v1/create-qr-code/"
                      "?size=130x130&color=0072ff&data=" +
                      encodedQr;

  file << "<!DOCTYPE html>\n"
       << "<html lang=\"en\">\n"
       << "<head>\n"
       << "    <meta charset=\"UTF-8\">\n"
       << "    <title>E-Ticket - " << booking.bookingId << "</title>\n"
       << "    <link "
          "href=\"https://fonts.googleapis.com/"
          "css2?family=Outfit:wght@300;400;600;800&display=swap\" "
          "rel=\"stylesheet\">\n"
       << "    <style>\n"
       << "        body { font-family: 'Outfit', sans-serif; background: "
          "#0b0f19; color: #fff; margin: 0; padding: 40px; display: flex; "
          "justify-content: center; align-items: center; min-height: 100vh; }\n"
       << "        .ticket-wrapper { display: flex; flex-direction: column; "
          "align-items: center; }\n"
       << "        .ticket { background: linear-gradient(135deg, #161f33 0%, "
          "#0d1321 100%); border-radius: 20px; box-shadow: 0 20px 40px "
          "rgba(0,0,0,0.5); width: 720px; overflow: hidden; border: 1px solid "
          "rgba(255,255,255,0.05); position: relative; }\n"
       << "        .header { background: linear-gradient(90deg, #00c6ff 0%, "
          "#0072ff 100%); padding: 30px; display: flex; justify-content: "
          "space-between; align-items: center; border-bottom: 2px dashed "
          "rgba(255,255,255,0.1); }\n"
       << "        .logo { font-size: 24px; font-weight: 800; letter-spacing: "
          "2px; color: #fff; }\n"
       << "        .logo span { color: #ffd700; }\n"
       << "        .badge { background: rgba(255,255,255,0.2); padding: 6px "
          "12px; border-radius: 30px; font-size: 12px; font-weight: 600; "
          "letter-spacing: 1px; text-transform: uppercase; }\n"
       << "        .ticket-body { display: flex; gap: 30px; padding: 40px; "
          "border-bottom: 1px solid rgba(255,255,255,0.05); }\n"
       << "        .ticket-main { flex: 1; }\n"
       << "        .ticket-sidebar { width: 170px; display: flex; "
          "flex-direction: column; align-items: center; justify-content: "
          "flex-start; border-left: 1px dashed rgba(255,255,255,0.1); "
          "padding-left: 30px; }\n"
       << "        .route { display: flex; justify-content: space-between; "
          "align-items: center; margin-bottom: 30px; }\n"
       << "        .city { font-size: 26px; font-weight: 800; color: #fff; }\n"
       << "        .arrow { font-size: 24px; color: #00c6ff; }\n"
       << "        .details-grid { display: grid; grid-template-columns: "
          "repeat(2, 1fr); gap: 20px; margin-bottom: 30px; border-bottom: 1px "
          "solid rgba(255,255,255,0.05); padding-bottom: 24px; }\n"
       << "        .label { font-size: 11px; text-transform: uppercase; color: "
          "#8f9cae; letter-spacing: 1px; margin-bottom: 4px; }\n"
       << "        .value { font-size: 15px; font-weight: 600; color: #fff; }\n"
       << "        .passengers { background: rgba(255,255,255,0.02); border: "
          "1px solid rgba(255,255,255,0.05); border-radius: 12px; padding: "
          "18px; }\n"
       << "        .p-title { font-size: 13px; font-weight: 800; "
          "text-transform: uppercase; color: #00c6ff; margin-bottom: 12px; "
          "border-bottom: 1px solid rgba(0,198,255,0.2); padding-bottom: 6px; "
          "}\n"
       << "        .p-row { display: flex; justify-content: space-between; "
          "margin-bottom: 8px; font-size: 13px; }\n"
       << "        .p-row:last-child { margin-bottom: 0; }\n"
       << "        .qr-wrapper { display: flex; flex-direction: column; "
          "align-items: center; background: rgba(255, 255, 255, 0.03); border: "
          "1px solid rgba(255, 255, 255, 0.05); border-radius: 16px; padding: "
          "16px; width: 100%; box-sizing: border-box; }\n"
       << "        .qr-code { width: 130px; height: 130px; border: 4px solid "
          "#fff; border-radius: 8px; background: #fff; }\n"
       << "        .qr-label { font-size: 10px; text-transform: uppercase; "
          "color: #00c6ff; letter-spacing: 1.5px; margin-top: 12px; "
          "font-weight: 800; text-align: center; }\n"
       << "        .qr-desc { font-size: 9px; color: #8f9cae; margin-top: 4px; "
          "text-align: center; line-height: 1.3; }\n"
       << "        .footer { background: #090d16; padding: 30px 40px; display: "
          "flex; justify-content: space-between; align-items: center; }\n"
       << "        .barcode-container { display: flex; flex-direction: column; "
          "align-items: flex-end; }\n"
       << "        .barcode { width: 150px; height: 45px; background: "
          "linear-gradient(90deg, #fff 2px, transparent 2px, transparent 4px, "
          "#fff 4px, #fff 7px, transparent 7px, transparent 9px, #fff 9px, "
          "#fff 11px, transparent 11px, #fff 15px, transparent 15px); "
          "background-size: 20px 100%; filter: invert(1); opacity: 0.8; }\n"
       << "        .fare-info { display: flex; flex-direction: column; }\n"
       << "        .total-label { font-size: 12px; text-transform: uppercase; "
          "color: #8f9cae; }\n"
       << "        .total-amount { font-size: 26px; font-weight: 800; color: "
          "#00ff88; }\n"
       << "        .print-btn { background: #0072ff; border: none; color: "
          "#fff; padding: 12px 24px; font-family: 'Outfit', sans-serif; "
          "font-weight: 600; border-radius: 8px; cursor: pointer; margin-top: "
          "20px; transition: background 0.3s; display: block; width: 100%; "
          "text-align: center; text-decoration: none; }\n"
       << "        .print-btn:hover { background: #00c6ff; }\n"
       << "        @media print {\n"
       << "            .print-btn { display: none; }\n"
       << "            body { padding: 0; background: #fff; color: #000; }\n"
       << "            .ticket { box-shadow: none; border: 1px solid #ccc; "
          "background: #fff; color: #000; width: 100%; }\n"
       << "            .value, .city { color: #000; }\n"
       << "            .header { background: #eee; border-bottom: 2px dashed "
          "#999; }\n"
       << "            .footer { background: #fafafa; }\n"
       << "            .total-amount { color: #000; }\n"
       << "            .ticket-sidebar { border-left: 1px dashed #999; }\n"
       << "            .qr-wrapper { background: none; border: none; }\n"
       << "            .qr-code { border: 2px solid #000; }\n"
       << "            .barcode { filter: none; }\n"
       << "        }\n"
       << "    </style>\n"
       << "</head>\n"
       << "<body>\n"
       << "    <div class=\"ticket-wrapper\">\n"
       << "        <div class=\"ticket\">\n"
       << "            <div class=\"header\">\n"
       << "                <div "
          "class=\"logo\">AEROLINE<span>TRAVELS</span></div>\n"
       << "                <div class=\"badge\">" << booking.status
       << "</div>\n"
       << "            </div>\n"
       << "            <div class=\"ticket-body\">\n"
       << "                <div class=\"ticket-main\">\n"
       << "                    <div class=\"route\">\n"
       << "                        <div>\n"
       << "                            <div class=\"label\">Origin</div>\n"
       << "                            <div class=\"city\">" << bus.source
       << "</div>\n"
       << "                        </div>\n"
       << "                        <div class=\"arrow\">➝</div>\n"
       << "                        <div style=\"text-align: right;\">\n"
       << "                            <div class=\"label\">Destination</div>\n"
       << "                            <div class=\"city\">" << bus.destination
       << "</div>\n"
       << "                        </div>\n"
       << "                    </div>\n"
       << "                    <div class=\"details-grid\">\n"
       << "                        <div>\n"
       << "                            <div class=\"label\">Booking "
          "Reference</div>\n"
       << "                            <div class=\"value\">"
       << booking.bookingId << "</div>\n"
       << "                        </div>\n"
       << "                        <div>\n"
       << "                            <div class=\"label\">Travel Date & "
          "Time</div>\n"
       << "                            <div class=\"value\">" << booking.date
       << " | " << bus.depTime << "</div>\n"
       << "                        </div>\n"
       << "                        <div>\n"
       << "                            <div class=\"label\">Bus Service</div>\n"
       << "                            <div class=\"value\">" << bus.busName
       << " (" << bus.busType << ")</div>\n"
       << "                        </div>\n"
       << "                        <div>\n"
       << "                            <div class=\"label\">Seats "
          "Reserved</div>\n"
       << "                            <div class=\"value\">";
  for (size_t i = 0; i < booking.seatNumbers.size(); ++i) {
    file << booking.seatNumbers[i]
         << (i + 1 < booking.seatNumbers.size() ? ", " : "");
  }
  file << "</div>\n"
       << "                        </div>\n"
       << "                    </div>\n"
       << "                    <div class=\"passengers\">\n"
       << "                        <div class=\"p-title\">Passenger "
          "Manifest</div>\n";
  for (size_t i = 0; i < booking.passengers.size(); ++i) {
    file << "                        <div class=\"p-row\">\n"
         << "                            <span>Seat " << booking.seatNumbers[i]
         << ": <strong>" << booking.passengers[i].name << "</strong></span>\n"
         << "                            <span>" << booking.passengers[i].age
         << " Years | " << booking.passengers[i].gender << "</span>\n"
         << "                        </div>\n";
  }
  file
      << "                    </div>\n"
      << "                </div>\n"
      << "                <div class=\"ticket-sidebar\">\n"
      << "                    <div class=\"qr-wrapper\">\n"
      << "                        <img class=\"qr-code\" src=\"" << qrUrl
      << "\" alt=\"Security Boarding QR\">\n"
      << "                        <div class=\"qr-label\">Security Pass</div>\n"
      << "                        <div class=\"qr-desc\">Scan at gate for "
         "digital check-in</div>\n"
      << "                    </div>\n"
      << "                </div>\n"
      << "            </div>\n"
      << "            <div class=\"footer\">\n"
      << "                <div class=\"fare-info\">\n"
      << "                    <div class=\"total-label\">Total Price</div>\n"
      << "                    <div class=\"total-amount\">Rs. " << std::fixed
      << std::setprecision(2) << booking.farePaid << "</div>\n"
      << "                </div>\n"
      << "                <div class=\"barcode-container\">\n"
      << "                    <div class=\"barcode\"></div>\n"
      << "                    <div style=\"font-size: 10px; color: #8f9cae; "
         "margin-top: 4px; letter-spacing: 2px;\">"
      << booking.bookingId << "</div>\n"
      << "                </div>\n"
      << "            </div>\n"
      << "        </div>\n"
      << "        <button class=\"print-btn\" onclick=\"window.print()\">Print "
         "E-Ticket as PDF</button>\n"
      << "    </div>\n"
      << "</body>\n"
      << "</html>\n";
  file.close();
}

std::vector<std::string> split(const std::string &s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

std::string trim(const std::string &str) {
  size_t first = str.find_first_not_of(" \t\r\n");
  if (first == std::string::npos)
    return "";
  size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, (last - first + 1));
}

std::string toUpper(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
  return str;
}

std::string toLower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  return str;
}

// Levenshtein edit-distance for fuzzy city name matching
int editDistance(const std::string &a, const std::string &b) {
  int m = (int)a.size(), n = (int)b.size();
  std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
  for (int i = 0; i <= m; ++i)
    dp[i][0] = i;
  for (int j = 0; j <= n; ++j)
    dp[0][j] = j;
  for (int i = 1; i <= m; ++i)
    for (int j = 1; j <= n; ++j)
      dp[i][j] =
          (a[i - 1] == b[j - 1])
              ? dp[i - 1][j - 1]
              : 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
  return dp[m][n];
}

// Returns true if userInput fuzzy-matches cityName
// Tier 1: exact (case-insensitive)
// Tier 2: city contains the user's input as substring (or vice versa)
// Tier 3: edit distance <= 2 (tolerates 2 typos / transpositions)
bool cityMatches(const std::string &cityName, const std::string &userInput) {
  std::string city = toLower(cityName);
  std::string query = toLower(userInput);
  if (city == query)
    return true; // exact
  if (city.find(query) != std::string::npos)
    return true; // city contains query
  if (query.find(city) != std::string::npos)
    return true; // query contains city
  int dist = editDistance(city, query);
  // Allow 1 typo for short names (<=6 chars), 2 for longer ones
  int threshold = (query.size() <= 6) ? 1 : 2;
  return dist <= threshold;
}

std::string sanitizeDelimiters(std::string str) {
  str.erase(std::remove(str.begin(), str.end(), '|'), str.end());
  str.erase(std::remove(str.begin(), str.end(), ':'), str.end());
  str.erase(std::remove(str.begin(), str.end(), ';'), str.end());
  return str;
}

std::string sanitizePhoneNumber(const std::string &phone) {
  std::string clean;
  for (char c : phone) {
    if (std::isdigit(c)) {
      clean.push_back(c);
    }
  }
  return clean;
}

std::string getMaskedPassword(const std::string &prompt) {
  while (true) {
    std::string cleanPrompt = prompt;
    size_t start = cleanPrompt.find_first_not_of(" \t");
    if (start != std::string::npos) {
      cleanPrompt = cleanPrompt.substr(start);
    }
    std::cout << Color::LIGHT_CYAN << "  > " << Color::WHITE << cleanPrompt
              << Color::RESET;
    std::string password;
    char ch;
    while (true) {
      ch = getConsoleChar();
      if (ch == '\r' || ch == '\n') {
        break;
      } else if (ch == '\b' || ch == 127) { // backspace
        if (!password.empty()) {
          password.pop_back();
          std::cout << "\b \b";
        }
      } else if (ch >= 32 && ch <= 126) {
        password.push_back(ch);
        std::cout << '*';
      }
    }
    std::cout << std::endl;

    if (password.find_first_of("|:;") != std::string::npos) {
      std::cout << Color::RED
                << "  [!] Password cannot contain delimiters (|, :, ;). Please "
                   "try again."
                << Color::RESET << std::endl;
      continue;
    }
    if (!password.empty()) {
      return password;
    }
    std::cout << Color::RED
              << "  [!] Password cannot be empty. Please try again."
              << Color::RESET << std::endl;
  }
}

// Simple and deterministic hash for passwords
std::string hashPassword(const std::string &password) {
  unsigned long hash = 5381;
  for (char c : password) {
    hash = ((hash << 5) + hash) + c;
  }
  std::stringstream ss;
  ss << std::hex << hash;
  return ss.str();
}

std::string getCurrentDate() {
  std::time_t t = std::time(nullptr);
  std::tm *now = std::localtime(&t);
  std::stringstream ss;
  ss << (now->tm_year + 1900) << "-" << std::setw(2) << std::setfill('0')
     << (now->tm_mon + 1) << "-" << std::setw(2) << std::setfill('0')
     << now->tm_mday;
  return ss.str();
}

std::string getCurrentTimestamp() {
  std::time_t t = std::time(nullptr);
  std::tm *now = std::localtime(&t);
  std::stringstream ss;
  ss << (now->tm_year + 1900) << "-" << std::setw(2) << std::setfill('0')
     << (now->tm_mon + 1) << "-" << std::setw(2) << std::setfill('0')
     << now->tm_mday << " " << std::setw(2) << std::setfill('0') << now->tm_hour
     << ":" << std::setw(2) << std::setfill('0') << now->tm_min << ":"
     << std::setw(2) << std::setfill('0') << now->tm_sec;
  return ss.str();
}

bool isValidDate(const std::string &dateStr) {
  if (dateStr.length() != 10 || dateStr[4] != '-' || dateStr[7] != '-')
    return false;

  int y, m, d;
  char dash1, dash2;
  std::stringstream ss(dateStr);
  if (!(ss >> y >> dash1 >> m >> dash2 >> d))
    return false;
  if (y < 2026 || y > 2100)
    return false;
  if (m < 1 || m > 12)
    return false;

  int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
    daysInMonth[1] = 29;
  }
  if (d < 1 || d > daysInMonth[m - 1])
    return false;

  return true;
}

// Progress loader simulator - optimized for speed
void showLoader(const std::string &label, int steps = 10, int sleepMs = 15) {
  std::cout << Color::BOLD << Color::CYAN << "  > " << label << " "
            << Color::RESET;
  std::cout << Color::BRIGHT_BLACK << "[";
  for (int i = 0; i < steps; ++i) {
    std::cout << Color::LIGHT_CYAN << "=";
    std::cout.flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
  }
  std::cout << Color::BRIGHT_BLACK << "]" << Color::BOLD << Color::GREEN
            << " Done!" << Color::RESET << std::endl;
}

// Custom prompt functions with input validation
void clearInput() {
  std::cin.clear();
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string getValidString(const std::string &prompt) {
  std::string input;
  while (true) {
    std::string cleanPrompt = prompt;
    size_t start = cleanPrompt.find_first_not_of(" \t");
    if (start != std::string::npos) {
      cleanPrompt = cleanPrompt.substr(start);
    }
    std::cout << Color::LIGHT_CYAN << "  > " << Color::WHITE << cleanPrompt
              << Color::RESET;
    std::getline(std::cin, input);
    input = trim(input);
    if (input.empty()) {
      std::cout << Color::RED
                << "  [!] Input cannot be empty. Please try again."
                << Color::RESET << std::endl;
      continue;
    }
    if (input.find_first_of("|:;") != std::string::npos) {
      std::cout << Color::RED
                << "  [!] Input cannot contain delimiters (|, :, ;). Please "
                   "try again."
                << Color::RESET << std::endl;
      continue;
    }
    return input;
  }
}

int getValidInt(const std::string &prompt,
                int minVal = std::numeric_limits<int>::min(),
                int maxVal = std::numeric_limits<int>::max()) {
  std::string line;
  int value;
  while (true) {
    std::string cleanPrompt = prompt;
    size_t start = cleanPrompt.find_first_not_of(" \t");
    if (start != std::string::npos) {
      cleanPrompt = cleanPrompt.substr(start);
    }
    std::cout << Color::LIGHT_CYAN << "  > " << Color::WHITE << cleanPrompt
              << Color::RESET;
    std::getline(std::cin, line);
    line = trim(line);
    std::stringstream ss(line);
    if (ss >> value && ss.eof()) {
      if (value >= minVal && value <= maxVal) {
        return value;
      } else {
        std::cout << Color::RED << "  [!] Input out of range (" << minVal
                  << " to " << maxVal << "). Please try again." << Color::RESET
                  << std::endl;
      }
    } else {
      std::cout << Color::RED
                << "  [!] Invalid integer input. Please try again."
                << Color::RESET << std::endl;
    }
  }
}

double getValidDouble(const std::string &prompt,
                      double minVal = std::numeric_limits<double>::lowest(),
                      double maxVal = std::numeric_limits<double>::max()) {
  std::string line;
  double value;
  while (true) {
    std::string cleanPrompt = prompt;
    size_t start = cleanPrompt.find_first_not_of(" \t");
    if (start != std::string::npos) {
      cleanPrompt = cleanPrompt.substr(start);
    }
    std::cout << Color::LIGHT_CYAN << "  > " << Color::WHITE << cleanPrompt
              << Color::RESET;
    std::getline(std::cin, line);
    line = trim(line);
    std::stringstream ss(line);
    if (ss >> value && ss.eof()) {
      if (value >= minVal && value <= maxVal) {
        return value;
      } else {
        std::cout << Color::RED << "  [!] Input out of range (" << minVal
                  << " to " << maxVal << "). Please try again." << Color::RESET
                  << std::endl;
      }
    } else {
      std::cout << Color::RED
                << "  [!] Invalid decimal input. Please try again."
                << Color::RESET << std::endl;
    }
  }
}

char getValidGender(const std::string &prompt) {
  std::string input;
  while (true) {
    input = toUpper(getValidString(prompt));
    if (input == "M" || input == "F" || input == "O") {
      return input[0];
    }
    std::cout << Color::RED << "Invalid option. Please enter M, F, or O."
              << Color::RESET << std::endl;
  }
}

std::string getFutureDateInput(const std::string &prompt) {
  std::string dateStr;
  std::string today = getCurrentDate();
  while (true) {
    dateStr = getValidString(prompt);
    if (isValidDate(dateStr)) {
      if (dateStr >= today) {
        return dateStr;
      } else {
        std::cout << Color::RED
                  << "Date of journey cannot be in the past (Today is " << today
                  << "). Try again." << Color::RESET << std::endl;
      }
    } else {
      std::cout << Color::RED
                << "Invalid date format. Use YYYY-MM-DD. Try again."
                << Color::RESET << std::endl;
    }
  }
}

// -------------------------------------------------------------
// Database/File Manager
// -------------------------------------------------------------

class BusBookingSystem {
private:
  std::vector<User> users;
  std::vector<Bus> buses;
  std::vector<Booking> bookings;
  std::vector<RouteRequest> routeRequests;
  User currentUser;
  bool isLoggedIn = false;

  // File Paths
  const std::string usersFile = "users.txt";
  const std::string busesFile = "buses.txt";
  const std::string bookingsFile = "bookings.txt";
  const std::string routeRequestsFile = "route_requests.txt";

public:
  BusBookingSystem() {
    enableVirtualTerminal();
    loadData();
  }

  void loadData() {
    loadUsers();
    loadBuses();
    loadBookings();
    loadRouteRequests();
  }

  void saveData() {
    saveUsers();
    saveBuses();
    saveBookings();
    saveRouteRequests();
  }

  // User Operations
  void loadUsers() {
    users.clear();
    std::ifstream file(usersFile);
    if (!file.is_open()) {
      // Seed default Admin & Customer if file doesn't exist
      User admin = {"admin", hashPassword("admin123"), "System Admin",
                    "+919876543210", "ADMIN"};
      User cust = {"soham", hashPassword("soham123"), "Soham Bahirat",
                   "+919988776655", "CUSTOMER"};
      users.push_back(admin);
      users.push_back(cust);
      saveUsers();
      return;
    }

    std::string line;
    while (std::getline(file, line)) {
      line = trim(line);
      if (line.empty())
        continue;
      std::vector<std::string> parts = split(line, '|');
      if (parts.size() == 5) {
        User u;
        u.username = parts[0];
        u.passwordHash = parts[1];
        u.fullName = parts[2];
        u.phone = parts[3];
        u.role = parts[4];
        users.push_back(u);
      }
    }
    file.close();
  }

  void saveUsers() {
    std::ofstream file(usersFile);
    for (const auto &u : users) {
      file << sanitizeDelimiters(u.username) << "|"
           << sanitizeDelimiters(u.passwordHash) << "|"
           << sanitizeDelimiters(u.fullName) << "|"
           << sanitizeDelimiters(u.phone) << "|" << sanitizeDelimiters(u.role)
           << "\n";
    }
    file.close();
  }

  // Bus Operations
  void loadBuses() {
    buses.clear();
    std::ifstream file(busesFile);
    if (!file.is_open()) {
      std::cout << Color::CYAN
                << "\n  [NET] Synchronizing live bus routes from central "
                   "server (dpaste)..."
                << Color::RESET << std::endl;

      // Execute powershell command to download the JSON from the internet
      std::string cmd = "powershell -Command \"Invoke-WebRequest -Uri "
                        "'https://dpaste.com/4R84335CT.txt' -OutFile "
                        "'online_buses.json' -ErrorAction SilentlyContinue\"";
      std::system(cmd.c_str());

      std::ifstream onlineFile("online_buses.json");
      if (onlineFile.is_open()) {
        try {
          nlohmann::json j;
          onlineFile >> j;
          for (const auto &item : j["buses"]) {
            Bus b;
            b.busId = item["busId"];
            b.busName = item["busName"];
            b.busType = item["busType"];
            b.source = item["source"];
            b.destination = item["destination"];
            b.depTime = item["depTime"];
            b.arrTime = item["arrTime"];
            b.fare = item["fare"];
            b.totalSeats = item["totalSeats"];
            buses.push_back(b);
          }
          std::cout << Color::GREEN
                    << "  [NET] Successfully fetched and parsed "
                    << buses.size() << " realistic routes!" << Color::RESET
                    << std::endl;
        } catch (const std::exception &e) {
          std::cout << Color::RED << "  [NET] JSON Parsing Error: " << e.what()
                    << Color::RESET << std::endl;
        }
        onlineFile.close();
        std::remove("online_buses.json");
      } else {
        std::cout
            << Color::RED
            << "  [NET] Failed to reach the internet. Loading local fallback..."
            << Color::RESET << std::endl;
        buses.push_back({"BUS-101", "Fallback Express", "AC Seater", "Mumbai",
                         "Pune", "10:00 AM", "02:00 PM", 500.0, 40});
      }

      saveBuses();
      return;
    }

    std::string line;
    while (std::getline(file, line)) {
      line = trim(line);
      if (line.empty())
        continue;
      std::vector<std::string> parts = split(line, '|');
      if (parts.size() == 9) {
        Bus b;
        b.busId = parts[0];
        b.busName = parts[1];
        b.busType = parts[2];
        b.source = parts[3];
        b.destination = parts[4];
        b.depTime = parts[5];
        b.arrTime = parts[6];
        b.fare = std::stod(parts[7]);
        b.totalSeats = std::stoi(parts[8]);
        buses.push_back(b);
      }
    }
    file.close();
  }

  void saveBuses() {
    std::ofstream file(busesFile);
    for (const auto &b : buses) {
      file << sanitizeDelimiters(b.busId) << "|"
           << sanitizeDelimiters(b.busName) << "|"
           << sanitizeDelimiters(b.busType) << "|"
           << sanitizeDelimiters(b.source) << "|"
           << sanitizeDelimiters(b.destination) << "|"
           << sanitizeDelimiters(b.depTime) << "|"
           << sanitizeDelimiters(b.arrTime) << "|" << std::fixed
           << std::setprecision(2) << b.fare << "|" << b.totalSeats << "\n";
    }
    file.close();
  }

  // Booking Operations
  void loadBookings() {
    bookings.clear();
    std::ifstream file(bookingsFile);
    if (!file.is_open())
      return;

    std::string line;
    while (std::getline(file, line)) {
      line = trim(line);
      if (line.empty())
        continue;
      std::vector<std::string> parts = split(line, '|');
      if (parts.size() >= 9) {
        Booking bk;
        bk.bookingId = parts[0];
        bk.username = parts[1];
        bk.busId = parts[2];
        bk.date = parts[3];

        // Parse seats
        std::vector<std::string> seatsStr = split(parts[4], ',');
        for (const auto &s : seatsStr) {
          if (!s.empty())
            bk.seatNumbers.push_back(std::stoi(s));
        }

        // Parse passengers
        std::vector<std::string> passStr = split(parts[5], ';');
        for (const auto &p : passStr) {
          if (p.empty())
            continue;
          std::vector<std::string> pParts = split(p, ':');
          if (pParts.size() == 3) {
            Passenger passenger;
            passenger.name = pParts[0];
            passenger.age = std::stoi(pParts[1]);
            passenger.gender = pParts[2][0];
            bk.passengers.push_back(passenger);
          }
        }

        bk.farePaid = std::stod(parts[6]);
        bk.status = parts[7];
        bk.timestamp = parts[8];
        bookings.push_back(bk);
      }
    }
    file.close();
  }

  void saveBookings() {
    std::ofstream file(bookingsFile);
    for (const auto &bk : bookings) {
      file << sanitizeDelimiters(bk.bookingId) << "|"
           << sanitizeDelimiters(bk.username) << "|"
           << sanitizeDelimiters(bk.busId) << "|" << sanitizeDelimiters(bk.date)
           << "|";

      // Seats
      for (size_t i = 0; i < bk.seatNumbers.size(); ++i) {
        file << bk.seatNumbers[i] << (i + 1 < bk.seatNumbers.size() ? "," : "");
      }
      file << "|";

      // Passengers
      for (size_t i = 0; i < bk.passengers.size(); ++i) {
        std::string pName = bk.passengers[i].name;
        pName.erase(std::remove(pName.begin(), pName.end(), ':'), pName.end());
        pName.erase(std::remove(pName.begin(), pName.end(), ';'), pName.end());
        pName.erase(std::remove(pName.begin(), pName.end(), '|'), pName.end());

        file << pName << ":" << bk.passengers[i].age << ":"
             << bk.passengers[i].gender
             << (i + 1 < bk.passengers.size() ? ";" : "");
      }
      file << "|";

      file << std::fixed << std::setprecision(2) << bk.farePaid << "|"
           << sanitizeDelimiters(bk.status) << "|"
           << sanitizeDelimiters(bk.timestamp) << "\n";
    }
    file.close();
  }

  // Route Request Operations
  void loadRouteRequests() {
    routeRequests.clear();
    std::ifstream file(routeRequestsFile);
    if (!file.is_open())
      return;
    std::string line;
    while (std::getline(file, line)) {
      line = trim(line);
      if (line.empty())
        continue;
      std::vector<std::string> parts = split(line, '|');
      if (parts.size() == 8) {
        RouteRequest rr;
        rr.requestId    = parts[0];
        rr.username     = parts[1];
        rr.source       = parts[2];
        rr.destination  = parts[3];
        rr.preferredTime = parts[4];
        rr.notes        = parts[5];
        rr.status       = parts[6];
        rr.timestamp    = parts[7];
        routeRequests.push_back(rr);
      }
    }
    file.close();
  }

  void saveRouteRequests() {
    std::ofstream file(routeRequestsFile);
    for (const auto &rr : routeRequests) {
      file << sanitizeDelimiters(rr.requestId)   << "|"
           << sanitizeDelimiters(rr.username)     << "|"
           << sanitizeDelimiters(rr.source)       << "|"
           << sanitizeDelimiters(rr.destination)  << "|"
           << sanitizeDelimiters(rr.preferredTime)<< "|"
           << sanitizeDelimiters(rr.notes)        << "|"
           << sanitizeDelimiters(rr.status)       << "|"
           << sanitizeDelimiters(rr.timestamp)    << "\n";
    }
    file.close();
  }

  // -------------------------------------------------------------
  // Core App Workflow Interfaces
  // -------------------------------------------------------------

  void showHeader() const {
    std::cout << Color::LIGHT_CYAN
              << " ╔═══════════════════════════════════════════════════════════"
                 "═══════════════╗"
              << std::endl;
    std::cout << " ║" << Color::CYAN
              << "   ___  _____  ___   ___  _     ___ _   _ _____   _______  "
                 "_____ ___  ___   "
              << Color::LIGHT_CYAN << "║" << std::endl;
    std::cout << " ║" << Color::CYAN
              << "  / _ \\|  ___|/ _ \\ / _ \\| |   |_ _| \\ | |  ___|  \\ \\ "
                 "/ //  ___/  _ \\/  _ \\  "
              << Color::LIGHT_CYAN << "║" << std::endl;
    std::cout << " ║" << Color::CYAN
              << " / /_\\ \\ |__ |   / | / \\ | |    | ||  \\| | |__     \\ V "
                 "/ \\ `--.| / \\ | / \\ | "
              << Color::LIGHT_CYAN << "║" << std::endl;
    std::cout << " ║" << Color::CYAN
              << " |  _  |  __||  _ \\| \\_/ | |___ | || |\\  |  __|     | |   "
                 "`--. \\ \\_/ | \\_/ | "
              << Color::LIGHT_CYAN << "║" << std::endl;
    std::cout << " ║" << Color::CYAN
              << " |_| |_|____||_| \\_\\\\___/|_____|___|_| \\_|____|     |_|  "
                 "\\____/ \\___/ \\___/  "
              << Color::LIGHT_CYAN << "║" << std::endl;
    std::cout << " ║                                                           "
                 "               ║"
              << std::endl;
    std::cout << " ║                  " << Color::BOLD << Color::YELLOW
              << "* * *  A E R O L I N E   T R A V E L S  * * *" << Color::RESET
              << Color::LIGHT_CYAN << "                 ║" << std::endl;
    std::cout << " ╚═══════════════════════════════════════════════════════════"
                 "═══════════════╝"
              << Color::RESET << std::endl;
  }

  void handleStartMenu() {
    while (!isLoggedIn) {
      clearScreen();
      showHeader();
      std::cout
          << Color::BRIGHT_BLACK
          << "  ┌────────────────────────────────────────────────────────┐"
          << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│" << Color::BOLD
                << Color::WHITE
                << "                   [ MAIN PORTAL MENU ]                  "
                << Color::BRIGHT_BLACK << "│" << Color::RESET << std::endl;
      std::cout
          << Color::BRIGHT_BLACK
          << "  ├────────────────────────────────────────────────────────┤"
          << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_BLUE
                << "1. " << Color::RESET << std::left << std::setw(51)
                << "Customer Login" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_BLUE
                << "2. " << Color::RESET << std::left << std::setw(51)
                << "Register Account" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_BLUE
                << "3. " << Color::RESET << std::left << std::setw(51)
                << "Administrator Console" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::RED << "4. "
                << Color::RESET << std::left << std::setw(51)
                << "Exit Application" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout
          << Color::BRIGHT_BLACK
          << "  └────────────────────────────────────────────────────────┘"
          << Color::RESET << std::endl;
      std::cout << std::endl;

      int choice = getValidInt("  Select Option (1-4): ", 1, 4);
      switch (choice) {
      case 1:
        handleLogin("CUSTOMER");
        break;
      case 2:
        handleRegister();
        break;
      case 3:
        handleLogin("ADMIN");
        break;
      case 4:
        clearScreen();
        std::cout
            << Color::BOLD << Color::GREEN
            << "\n  Thank you for choosing BusBook! Happy journey ahead!\n"
            << Color::RESET << std::endl;
        saveData();
        return;
      }
    }

    // Branch menu depending on active profile role
    if (currentUser.role == "ADMIN") {
      handleAdminMenu();
    } else {
      handleCustomerMenu();
    }
  }

  void handleLogin(const std::string &expectedRole) {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE << "\n  [ " << expectedRole
              << " LOGIN ]" << Color::RESET << std::endl;
    std::cout << "  ======================" << std::endl;
    std::string username = getValidString("  Enter Username: ");
    std::string password = getMaskedPassword("  Enter Password: ");

    std::string passHash = hashPassword(password);
    auto it = std::find_if(users.begin(), users.end(), [&](const User &u) {
      return u.username == username && u.passwordHash == passHash;
    });

    if (it != users.end()) {
      if (it->role == expectedRole) {
        currentUser = *it;
        isLoggedIn = true;
        showLoader("\n  Authenticating credentials...", 8, 15);
        std::cout << Color::GREEN << "  Access Granted! Welcome back, "
                  << currentUser.fullName << "." << Color::RESET << std::endl;
        std::cout << "\n  Press Enter to continue...";
        std::string dummy;
        std::getline(std::cin, dummy);
      } else {
        std::cout << Color::RED
                  << "\n  Error: Access Denied. Account is not registered as "
                  << expectedRole << "." << Color::RESET << std::endl;
        std::cout << "  Press Enter to return...";
        std::string dummy;
        std::getline(std::cin, dummy);
      }
    } else {
      std::cout << Color::RED << "\n  Error: Invalid username or password."
                << Color::RESET << std::endl;
      std::cout << "  Press Enter to return...";
      std::string dummy;
      std::getline(std::cin, dummy);
    }
  }

  void handleRegister() {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE
              << "\n  [ REGISTER NEW CUSTOMER ACCOUNT ]" << Color::RESET
              << std::endl;
    std::cout << "  ==================================" << std::endl;

    std::string username;
    while (true) {
      username = toLower(getValidString("  Choose Username: "));
      auto it = std::find_if(users.begin(), users.end(), [&](const User &u) {
        return u.username == username;
      });
      if (it == users.end())
        break;
      std::cout << Color::RED
                << "  Username already taken. Please choose another one."
                << Color::RESET << std::endl;
    }

    std::string password = getMaskedPassword("  Create Password: ");
    std::string fullName = getValidString("  Full Name: ");
    std::string phone = getValidString("  Phone Number: ");

    User newUser = {username, hashPassword(password), fullName, phone,
                    "CUSTOMER"};
    users.push_back(newUser);
    saveUsers();

    showLoader("\n  Creating profile and database entry...", 8, 15);
    std::cout << Color::GREEN
              << "  Account registered successfully! You can now log in."
              << Color::RESET << std::endl;
    std::cout << "\n  Press Enter to continue...";
    std::string dummy;
    std::getline(std::cin, dummy);
  }

  void handleLogout() {
    showLoader("\n  Saving transactions and signing out...", 8, 15);
    isLoggedIn = false;
    currentUser = User();
    std::cout << Color::GREEN << "  Successfully Logged Out!" << Color::RESET
              << std::endl;
    std::cout << "\n  Press Enter to return to main portal...";
    std::string dummy;
    std::getline(std::cin, dummy);
    handleStartMenu();
  }

  // -------------------------------------------------------------
  // Customer Dashboard Menu
  // -------------------------------------------------------------

  void handleCustomerMenu() {
    while (isLoggedIn) {
      clearScreen();
      showHeader();
      std::cout
          << Color::BRIGHT_BLACK
          << "  ┌────────────────────────────────────────────────────────┐"
          << Color::RESET << std::endl;
      std::stringstream title;
      title << "Welcome, " << currentUser.fullName << " (Customer)";
      std::cout << "  " << Color::BRIGHT_BLACK << "│ " << Color::BOLD
                << Color::YELLOW << std::left << std::setw(55) << title.str()
                << Color::BRIGHT_BLACK << "│" << Color::RESET << std::endl;
      std::cout
          << Color::BRIGHT_BLACK
          << "  ├────────────────────────────────────────────────────────┤"
          << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "1. " << Color::RESET << std::left << std::setw(51)
                << "Search Buses & Book Tickets" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "2. " << Color::RESET << std::left << std::setw(51)
                << "View My Tickets" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "3. " << Color::RESET << std::left << std::setw(51)
                << "Cancel a Booking" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "4. " << Color::RESET << std::left << std::setw(51)
                << "View Profile / Change Password" << Color::BRIGHT_BLACK
                << "│" << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "5. " << Color::RESET << std::left << std::setw(51)
                << "Reschedule a Booking" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "6. " << Color::RESET << std::left << std::setw(51)
                << "My Travel Stats & Loyalty Points" << Color::BRIGHT_BLACK
                << "│" << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "7. " << Color::RESET << std::left << std::setw(51)
                << "Request a New Bus Route" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::RED << "8. "
                << Color::RESET << std::left << std::setw(51) << "Logout"
                << Color::BRIGHT_BLACK << "│" << Color::RESET << std::endl;
      std::cout
          << Color::BRIGHT_BLACK
          << "  └────────────────────────────────────────────────────────┘"
          << Color::RESET << std::endl;
      std::cout << std::endl;

      int choice = getValidInt("  Choose Option (1-8): ", 1, 8);
      switch (choice) {
      case 1:
        bookTicketsFlow();
        break;
      case 2:
        viewCustomerTickets();
        break;
      case 3:
        cancelBookingFlow();
        break;
      case 4:
        viewCustomerProfile();
        break;
      case 5:
        rescheduleBookingFlow();
        break;
      case 6:
        viewTravelStats();
        break;
      case 7:
        requestRouteFlow();
        break;
      case 8:
        handleLogout();
        break;
      }
    }
  }

  void bookTicketsFlow() {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE
              << "\n  [ SEARCH FOR BUS DEPARTURES ]" << Color::RESET
              << std::endl;
    std::cout << "  =============================" << std::endl;

    std::string source = getValidString("  Origin City      : ");
    std::string destination = getValidString("  Destination City : ");
    std::string dateOfJourney =
        getFutureDateInput("  Journey Date (YYYY-MM-DD): ");

    // 3-tier fuzzy search: exact > substring > edit-distance typo tolerance
    std::vector<Bus> matchingBuses;
    std::string resolvedSource, resolvedDest;

    // Tier 1: Exact match (case-insensitive)
    for (const auto &b : buses) {
      if (toLower(b.source) == toLower(source) &&
          toLower(b.destination) == toLower(destination)) {
        matchingBuses.push_back(b);
        resolvedSource = b.source;
        resolvedDest = b.destination;
      }
    }

    // Tier 2 & 3: Fuzzy match if no exact results
    if (matchingBuses.empty()) {
      for (const auto &b : buses) {
        if (cityMatches(b.source, source) &&
            cityMatches(b.destination, destination)) {
          matchingBuses.push_back(b);
          resolvedSource = b.source;
          resolvedDest = b.destination;
        }
      }
      // If fuzzy found results, inform the user of the auto-correction
      if (!matchingBuses.empty()) {
        std::cout << Color::YELLOW << "\n  [*] Auto-corrected: '" << source
                  << "' → '" << resolvedSource << "', '" << destination
                  << "' → '" << resolvedDest << "'" << Color::RESET
                  << std::endl;
        std::cout << "      Showing results for the closest matching route."
                  << std::endl;
        // Small pause so user can read the correction
        std::this_thread::sleep_for(std::chrono::milliseconds(1200));
      }
    }

    if (matchingBuses.empty()) {
      // Show available cities as a hint
      std::set<std::string> allSources, allDests;
      for (const auto &b : buses) {
        allSources.insert(b.source);
        allDests.insert(b.destination);
      }
      std::cout << Color::RED << "\n  No buses found for the route '" << source
                << " to " << destination << "'." << Color::RESET << std::endl;
      std::cout << Color::YELLOW << "\n  Available Origin Cities : ";
      bool first = true;
      for (const auto &c : allSources) {
        if (!first)
          std::cout << ", ";
        std::cout << c;
        first = false;
      }
      std::cout << std::endl;
      std::cout << "  Available Destinations  : ";
      first = true;
      for (const auto &c : allDests) {
        if (!first)
          std::cout << ", ";
        std::cout << c;
        first = false;
      }
      std::cout << Color::RESET << std::endl;
      std::cout << "\n  Press Enter to try again...";
      std::string dummy;
      std::getline(std::cin, dummy);
      return;
    }

    // Print matches
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::GREEN << "\n  Buses Running From "
              << toUpper(source) << " to " << toUpper(destination) << " on "
              << dateOfJourney << ":" << Color::RESET << std::endl;
    std::cout << "  " << Color::BRIGHT_BLACK
              << "┌──────────┬──────────────────────┬───────────────┬──────────"
                 "┬──────────┬──────────────┐"
              << Color::RESET << std::endl;
    std::cout << "  " << Color::BRIGHT_BLACK << "│ " << Color::BOLD
              << Color::CYAN << std::left << std::setw(9) << "Bus ID"
              << Color::BRIGHT_BLACK << "│ " << Color::BOLD << Color::CYAN
              << std::setw(21) << "Bus Name" << Color::BRIGHT_BLACK << "│ "
              << Color::BOLD << Color::CYAN << std::setw(14) << "Type"
              << Color::BRIGHT_BLACK << "│ " << Color::BOLD << Color::CYAN
              << std::setw(9) << "Dept" << Color::BRIGHT_BLACK << "│ "
              << Color::BOLD << Color::CYAN << std::setw(9) << "Arrv"
              << Color::BRIGHT_BLACK << "│ " << Color::BOLD << Color::CYAN
              << std::setw(13) << "Fare (INR)" << Color::BRIGHT_BLACK << "│"
              << Color::RESET << std::endl;
    std::cout << "  " << Color::BRIGHT_BLACK
              << "├──────────┼──────────────────────┼───────────────┼──────────"
                 "┼──────────┼──────────────┤"
              << Color::RESET << std::endl;

    for (const auto &b : matchingBuses) {
      std::cout << "  " << Color::BRIGHT_BLACK << "│ " << Color::RESET
                << std::left << std::setw(9) << b.busId << Color::BRIGHT_BLACK
                << "│ " << Color::RESET << std::setw(21) << b.busName
                << Color::BRIGHT_BLACK << "│ " << Color::RESET << std::setw(14)
                << b.busType << Color::BRIGHT_BLACK << "│ " << Color::RESET
                << std::setw(9) << b.depTime << Color::BRIGHT_BLACK << "│ "
                << Color::RESET << std::setw(9) << b.arrTime
                << Color::BRIGHT_BLACK << "│ " << Color::RESET << Color::GREEN
                << "Rs. " << std::fixed << std::setprecision(2) << std::setw(8)
                << b.fare << Color::RESET << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
    }
    std::cout << "  " << Color::BRIGHT_BLACK
              << "└──────────┴──────────────────────┴───────────────┴──────────"
                 "┴──────────┴──────────────┘"
              << Color::RESET << std::endl;

    std::string selectedId;
    Bus selectedBus;
    bool found = false;
    while (!found) {
      selectedId = toUpper(
          getValidString("\n  Enter Bus ID to Book (or 'BACK' to cancel): "));
      if (selectedId == "BACK")
        return;

      for (const auto &b : matchingBuses) {
        if (b.busId == selectedId) {
          selectedBus = b;
          found = true;
          break;
        }
      }
      if (!found) {
        std::cout << Color::RED
                  << "  Invalid Bus ID. Please select from the list above."
                  << Color::RESET << std::endl;
      }
    }

    // Build occupied seats for this bus and date
    std::set<int> occupiedSeats;
    for (const auto &bk : bookings) {
      if (bk.busId == selectedBus.busId && bk.date == dateOfJourney &&
          bk.status == "BOOKED") {
        for (int seat : bk.seatNumbers) {
          occupiedSeats.insert(seat);
        }
      }
    }

    // Dynamic seat map renderer
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE
              << "\n  [ SEAT MAP FOR BUS: " << selectedBus.busName << " ("
              << selectedBus.busId << ") ]" << Color::RESET << std::endl;
    std::cout << "  Journey Date: " << dateOfJourney
              << " | Route: " << selectedBus.source << " -> "
              << selectedBus.destination << std::endl;
    std::cout << "  ========================================================"
              << std::endl;

    // Render layout: 4 seats per row with central aisle
    int totalSeats = selectedBus.totalSeats;
    int rows = totalSeats / 4;

    std::cout << "\n           [ FRONT / DRIVER ]" << std::endl;
    std::cout << "     ┌────────────────────────────┐" << std::endl;
    for (int r = 0; r < rows; ++r) {
      std::cout << "      ";
      for (int col = 0; col < 4; ++col) {
        int seatNum = (r * 4) + col + 1;
        bool isOccupied = occupiedSeats.count(seatNum) > 0;

        std::string seatStr = (seatNum < 10) ? ("0" + std::to_string(seatNum))
                                             : std::to_string(seatNum);

        if (isOccupied) {
          std::cout << Color::RED << "[" << Color::BOLD << "XX" << Color::RESET
                    << Color::RED << "]" << Color::RESET;
        } else {
          std::cout << Color::GREEN << "[" << Color::BOLD << seatStr
                    << Color::RESET << Color::GREEN << "]" << Color::RESET;
        }

        if (col == 1) {
          std::cout << "   |   "; // central aisle
        } else if (col != 3) {
          std::cout << " ";
        }
      }
      std::cout << std::endl;
    }
    std::cout << "     └────────────────────────────┘" << std::endl;
    std::cout << "  Legend: " << Color::GREEN << "[##] Available"
              << Color::RESET << "  " << Color::RED << "[XX] Occupied / Booked"
              << Color::RESET << std::endl;
    std::cout << "  ========================================================"
              << std::endl;

    // Seat selection
    int availableSeatsCount = totalSeats - occupiedSeats.size();
    if (availableSeatsCount <= 0) {
      std::cout << Color::RED
                << "\n  Sorry! This bus is fully booked for the selected date."
                << Color::RESET << std::endl;
      std::cout << "  Press Enter to return to menu...";
      std::string dummy;
      std::getline(std::cin, dummy);
      return;
    }

    int ticketsToBook =
        getValidInt("\n  Enter number of tickets to book (1-6): ", 1,
                    std::min(6, availableSeatsCount));
    std::vector<int> selectedSeats;
    for (int i = 0; i < ticketsToBook; ++i) {
      while (true) {
        std::stringstream prompt;
        prompt << "  Select Seat Number for passenger " << (i + 1) << " (1-"
               << totalSeats << "): ";
        int seat = getValidInt(prompt.str(), 1, totalSeats);

        if (occupiedSeats.count(seat) > 0) {
          std::cout << Color::RED << "  Seat " << seat
                    << " is already occupied. Choose another." << Color::RESET
                    << std::endl;
        } else if (std::find(selectedSeats.begin(), selectedSeats.end(),
                             seat) != selectedSeats.end()) {
          std::cout << Color::RED << "  You have already selected Seat " << seat
                    << "." << Color::RESET << std::endl;
        } else {
          selectedSeats.push_back(seat);
          break;
        }
      }
    }

    // Enter passengers details
    std::vector<Passenger> passengers;
    std::cout << Color::BOLD << Color::CYAN << "\n  [ PASSENGER DETAILS FORM ]"
              << Color::RESET << std::endl;
    for (int i = 0; i < ticketsToBook; ++i) {
      std::cout << Color::BOLD << "\n  Passenger #" << (i + 1) << " (Seat "
                << selectedSeats[i] << "):" << Color::RESET << std::endl;
      Passenger p;
      p.name = getValidString("    Full Name: ");
      p.age = getValidInt("    Age: ", 1, 120);
      p.gender = getValidGender("    Gender (M/F/O): ");
      passengers.push_back(p);
    }

    // Pricing summary
    double baseFare = selectedBus.fare * ticketsToBook;
    double tax = baseFare * 0.05; // 5% GST
    double totalFare = baseFare + tax;

    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::YELLOW << "\n  [ TRANSACTION SUMMARY ]"
              << Color::RESET << std::endl;
    std::cout << "  --------------------------------------------------------"
              << std::endl;
    std::cout << "  Bus Line    : " << selectedBus.busName << " ("
              << selectedBus.busId << ")" << std::endl;
    std::cout << "  Type        : " << selectedBus.busType << std::endl;
    std::cout << "  Route       : " << selectedBus.source << " -> "
              << selectedBus.destination << std::endl;
    std::cout << "  Date        : " << dateOfJourney
              << " | Time: " << selectedBus.depTime << std::endl;
    std::cout << "  Seats Selected: ";
    for (size_t i = 0; i < selectedSeats.size(); ++i) {
      std::cout << selectedSeats[i]
                << (i + 1 < selectedSeats.size() ? ", " : "");
    }
    std::cout << std::endl;
    std::cout << "  --------------------------------------------------------"
              << std::endl;
    std::cout << "  Base Price  : Rs. " << std::fixed << std::setprecision(2)
              << baseFare << std::endl;
    std::cout << "  Service GST (5%): Rs. " << tax << std::endl;
    std::cout << "  Total Amount: " << Color::GREEN << "Rs. " << totalFare
              << Color::RESET << std::endl;
    std::cout << "  --------------------------------------------------------"
              << std::endl;

    std::string confirm =
        toUpper(getValidString("  Proceed to payment? (Y/N): "));
    if (confirm != "Y") {
      std::cout << Color::RED << "\n  Transaction Cancelled." << Color::RESET
                << std::endl;
      std::cout << "  Press Enter to return to menu...";
      std::string dummy;
      std::getline(std::cin, dummy);
      return;
    }

    // Simulating Interactive Payment gateway
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::CYAN << "\n  [ SECURE PAYMENT GATEWAY ]"
              << Color::RESET << std::endl;
    std::cout << "  ===========================" << std::endl;
    std::cout << "  Amount Payable: " << Color::GREEN << "Rs. " << totalFare
              << Color::RESET << std::endl;
    std::cout << "\n  Select Payment Method:" << std::endl;
    std::cout << "  1. UPI Instant (GPay / PhonePe / Paytm)" << std::endl;
    std::cout << "  2. Credit / Debit Card" << std::endl;
    std::cout << "  3. Internet Banking" << std::endl;

    int payMethod = getValidInt("  Choose Payment Method (1-3): ", 1, 3);
    if (payMethod == 1) {
      std::cout << "\n  Simulating UPI payment request..." << std::endl;
      std::cout << "  " << Color::YELLOW
                << "[!] Scan QR Code or check notification on UPI app"
                << Color::RESET << std::endl;
      std::cout << "  UPI ID generated: " << currentUser.username << "@busbook"
                << std::endl;
      std::string enterUpi = getValidString(
          "  Press Enter once you approve payment in UPI App...");
      (void)enterUpi;
    } else if (payMethod == 2) {
      std::string card = getValidString("  Enter Card Number (16 Digits): ");
      std::string exp = getValidString("  Enter Expiry (MM/YY): ");
      std::string cvv = getMaskedPassword("  Enter CVV: ");
      (void)card;
      (void)exp;
      (void)cvv;

      // Random OTP Simulation
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<> dis(100000, 999999);
      int mockOtp = dis(gen);

      std::cout << Color::YELLOW
                << "\n  [!] Mock OTP sent to registered phone number: "
                << mockOtp << Color::RESET << std::endl;
      int otpEntered = getValidInt("  Enter OTP: ", 100000, 999999);

      while (otpEntered != mockOtp) {
        std::cout << Color::RED
                  << "  Incorrect OTP. Try again (Hint: check above code): "
                  << Color::RESET;
        otpEntered = getValidInt("", 100000, 999999);
      }
    } else {
      std::cout << "\n  Select Bank: 1. State Bank of India  2. HDFC Bank  3. "
                   "ICICI Bank"
                << std::endl;
      int bank = getValidInt("  Choose Bank (1-3): ", 1, 3);
      std::string uId = getValidString("  Enter NetBanking User ID: ");
      std::string pass = getMaskedPassword("  Enter password: ");
      (void)bank;
      (void)uId;
      (void)pass;
    }

    std::cout << std::endl;
    showLoader("  Verifying Funds and Authorizing Transaction...", 12, 20);
    showLoader("  Finalizing Ticket Reservation...", 8, 15);

    // Generate Booking Transaction
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);
    std::stringstream bkIdStream;
    bkIdStream << "TXN-" << dis(gen);

    Booking newBk;
    newBk.bookingId = bkIdStream.str();
    newBk.username = currentUser.username;
    newBk.busId = selectedBus.busId;
    newBk.date = dateOfJourney;
    newBk.seatNumbers = selectedSeats;
    newBk.passengers = passengers;
    newBk.farePaid = totalFare;
    newBk.status = "BOOKED";
    newBk.timestamp = getCurrentTimestamp();

    bookings.push_back(newBk);
    saveBookings();

    std::cout << Color::GREEN
              << "\n  Payment Successful! Ticket reserved successfully."
              << Color::RESET << std::endl;

    // Generate and open HTML E-Ticket
    generateHtmlTicket(newBk, selectedBus);
    std::string localPath = "tickets/ticket_" + newBk.bookingId + ".html";
    openBrowser(localPath);
    std::cout << Color::CYAN
              << "  [✓] Premium Boarding Pass E-Ticket opened in browser."
              << Color::RESET << std::endl;
    std::cout << "      (File saved locally to " << localPath << ")"
              << std::endl;

    // WhatsApp automatic redirect
    std::cout << std::endl;
    {
      std::string targetPhone = currentUser.phone;
      std::cout << "  [!] Automatically sending ticket confirmation to "
                   "registered WhatsApp ("
                << targetPhone << ")..." << std::endl;

      std::stringstream waMsg;
      waMsg << "*Aeroline Travels - Booking Confirmed* 🎫\n\n"
            << "Dear " << currentUser.fullName << ",\n"
            << "Your ticket has been confirmed. Details:\n"
            << "• *Booking ID:* " << newBk.bookingId << "\n"
            << "• *Bus:* " << selectedBus.busName << " (" << selectedBus.busType
            << ")\n"
            << "• *Route:* " << selectedBus.source << " -> "
            << selectedBus.destination << "\n"
            << "• *Date:* " << newBk.date << " (" << selectedBus.depTime
            << ")\n"
            << "• *Seats:* ";
      for (size_t i = 0; i < newBk.seatNumbers.size(); ++i) {
        waMsg << newBk.seatNumbers[i]
              << (i + 1 < newBk.seatNumbers.size() ? ", " : "");
      }
      waMsg << "\n• *Total Fare:* Rs. " << std::fixed << std::setprecision(2)
            << newBk.farePaid << "\n\n"
            << "Your PDF printable copy has been created locally in your "
               "workspace tickets directory.\n"
            << "Thank you for traveling with us! Safe journey! 🚌✨";

      std::string encodedMsg = urlEncode(waMsg.str());
      std::string waUrl =
          "whatsapp://send?phone=" + sanitizePhoneNumber(targetPhone) +
          "&text=" + encodedMsg;
      openBrowser(waUrl);
      std::cout << Color::GREEN
                << "  [✓] Launching installed WhatsApp Desktop app for number: "
                << targetPhone << Color::RESET << std::endl;

#ifdef _WIN32
      std::cout
          << Color::YELLOW
          << "  [!] Initializing automatic message transmission dispatch..."
          << Color::RESET << std::endl;
      std::cout << "      Launching background automation driver..."
                << std::endl;
      std::thread(runWhatsAppAutomation).detach();
#endif
    }

    std::cout << "\n  Press Enter to print receipt details in console...";
    std::string dummy;
    std::getline(std::cin, dummy);

    clearScreen();
    showHeader();
    std::cout << "\n";
    printReceipt(newBk, selectedBus);

    std::cout << "\n  Press Enter to go back to menu...";
    std::getline(std::cin, dummy);
  }

  void printReceipt(const Booking &booking, const Bus &bus) const {
    std::cout << Color::CYAN
              << "┌────────────────────────────────────────────────────────┐"
              << std::endl;
    std::cout << "│                    " << Color::BOLD << Color::YELLOW
              << "BOOKING RECEIPT" << Color::RESET << Color::CYAN
              << "                     │" << std::endl;
    std::cout << "├────────────────────────────────────────────────────────┤"
              << std::endl;
    std::cout << "│ Booking ID : " << std::left << std::setw(41)
              << booking.bookingId << "│" << std::endl;
    std::cout << "│ Status     : " << std::left << std::setw(41)
              << (booking.status == "BOOKED" ? "CONFIRMED" : "CANCELLED") << "│"
              << std::endl;
    std::cout << "│ User       : " << std::left << std::setw(41)
              << booking.username << "│" << std::endl;
    std::cout << "├────────────────────────────────────────────────────────┤"
              << std::endl;
    std::cout << "│ Bus        : " << std::left << std::setw(41)
              << (bus.busName + " (" + bus.busId + ")") << "│" << std::endl;
    std::cout << "│ Route      : " << std::left << std::setw(41)
              << (bus.source + " -> " + bus.destination) << "│" << std::endl;
    std::cout << "│ Date       : " << std::left << std::setw(41) << booking.date
              << "│" << std::endl;
    std::cout << "│ Departure  : " << std::left << std::setw(41) << bus.depTime
              << "│" << std::endl;
    std::cout << "├────────────────────────────────────────────────────────┤"
              << std::endl;
    std::cout << "│ Seats      : ";
    std::stringstream seatsStream;
    for (size_t i = 0; i < booking.seatNumbers.size(); ++i) {
      seatsStream << booking.seatNumbers[i]
                  << (i + 1 < booking.seatNumbers.size() ? ", " : "");
    }
    std::cout << std::left << std::setw(41) << seatsStream.str() << "│"
              << std::endl;
    std::cout << "├────────────────────────────────────────────────────────┤"
              << std::endl;
    std::cout << "│ Passengers:                                            │"
              << std::endl;
    for (size_t i = 0; i < booking.passengers.size(); ++i) {
      std::stringstream pInfo;
      pInfo << "Seat " << booking.seatNumbers[i] << ": "
            << booking.passengers[i].name << " (" << booking.passengers[i].age
            << ", " << booking.passengers[i].gender << ")";
      std::cout << "│  - " << std::left << std::setw(50) << pInfo.str() << "│"
                << std::endl;
    }
    std::cout << "├────────────────────────────────────────────────────────┤"
              << std::endl;
    std::cout << "│ Total Paid : " << Color::GREEN << "Rs. " << std::left
              << std::setw(37) << std::fixed << std::setprecision(2)
              << booking.farePaid << Color::RESET << Color::CYAN << "│"
              << std::endl;
    std::cout << "└────────────────────────────────────────────────────────┘"
              << Color::RESET << std::endl;
  }

  void viewCustomerTickets() const {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE << "\n  [ MY BOOKING HISTORY ]"
              << Color::RESET << std::endl;
    std::cout << "  ======================" << std::endl;

    std::vector<Booking> myBookings;
    for (const auto &bk : bookings) {
      if (bk.username == currentUser.username) {
        myBookings.push_back(bk);
      }
    }

    if (myBookings.empty()) {
      std::cout << Color::RED << "  No bookings found for your account."
                << Color::RESET << std::endl;
      std::cout << "\n  Press Enter to return to dashboard...";
      std::string dummy;
      std::getline(std::cin, dummy);
      return;
    }

    // Print summarized list
    std::cout << "  " << Color::BRIGHT_BLACK
              << "┌──────────────┬──────────┬──────────────┬──────────────┬────"
                 "──────────┬────────────┐"
              << Color::RESET << std::endl;
    std::cout << "  " << Color::BRIGHT_BLACK << "│ " << Color::BOLD
              << Color::CYAN << std::left << std::setw(13) << "Booking ID"
              << Color::BRIGHT_BLACK << "│ " << Color::BOLD << Color::CYAN
              << std::setw(9) << "Bus ID" << Color::BRIGHT_BLACK << "│ "
              << Color::BOLD << Color::CYAN << std::setw(13) << "Date"
              << Color::BRIGHT_BLACK << "│ " << Color::BOLD << Color::CYAN
              << std::setw(13) << "Fare Paid" << Color::BRIGHT_BLACK << "│ "
              << Color::BOLD << Color::CYAN << std::setw(13) << "Status"
              << Color::BRIGHT_BLACK << "│ " << Color::BOLD << Color::CYAN
              << std::setw(11) << "Timestamp" << Color::BRIGHT_BLACK << "│"
              << Color::RESET << std::endl;
    std::cout << "  " << Color::BRIGHT_BLACK
              << "├──────────────┼──────────┼──────────────┼──────────────┼────"
                 "──────────┼────────────┤"
              << Color::RESET << std::endl;

    for (const auto &bk : myBookings) {
      std::string statusStr = bk.status;
      if (bk.status == "BOOKED") {
        statusStr = Color::GREEN + "CONFIRMED" + Color::RESET;
      } else {
        statusStr = Color::RED + "CANCELLED" + Color::RESET;
      }
      std::cout << "  " << Color::BRIGHT_BLACK << "│ " << Color::RESET
                << std::left << std::setw(13) << bk.bookingId
                << Color::BRIGHT_BLACK << "│ " << Color::RESET << std::setw(9)
                << bk.busId << Color::BRIGHT_BLACK << "│ " << Color::RESET
                << std::setw(13) << bk.date << Color::BRIGHT_BLACK << "│ "
                << Color::RESET << Color::GREEN << "Rs. " << std::fixed
                << std::setprecision(2) << std::setw(9) << bk.farePaid
                << Color::RESET << Color::BRIGHT_BLACK << "│ " << Color::RESET
                << std::left << std::setw(22) << statusStr
                << Color::BRIGHT_BLACK << "│ " << Color::RESET << std::setw(11)
                << bk.timestamp.substr(0, 10) << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
    }
    std::cout << "  " << Color::BRIGHT_BLACK
              << "└──────────────┴──────────┴──────────────┴──────────────┴────"
                 "──────────┴────────────┘"
              << Color::RESET << std::endl;

    std::string viewId = toUpper(getValidString(
        "\n  Enter Booking ID to print details (or 'BACK' to exit): "));
    if (viewId == "BACK")
      return;

    auto it =
        std::find_if(myBookings.begin(), myBookings.end(),
                     [&](const Booking &bk) { return bk.bookingId == viewId; });

    if (it != myBookings.end()) {
      // Find Bus details
      Bus busObj;
      for (const auto &b : buses) {
        if (b.busId == it->busId) {
          busObj = b;
          break;
        }
      }

      clearScreen();
      showHeader();
      std::cout << "\n";
      printReceipt(*it, busObj);

      std::cout << "\n  Ticket Options:" << std::endl;
      std::cout << "  1. Regenerate & Print PDF E-Ticket (Opens in Browser)"
                << std::endl;
      std::cout << "  2. Send Ticket Confirmation to WhatsApp" << std::endl;
      std::cout << "  3. Go Back" << std::endl;
      int opt = getValidInt("  Choose Option (1-3): ", 1, 3);
      if (opt == 1) {
        generateHtmlTicket(*it, busObj);
        std::string localPath = "tickets/ticket_" + it->bookingId + ".html";
        openBrowser(localPath);
        std::cout << Color::GREEN
                  << "  [✓] PDF printable ticket opened in browser."
                  << Color::RESET << std::endl;
        std::cout << "\n  Press Enter to continue...";
        std::string dummy;
        std::getline(std::cin, dummy);
      } else if (opt == 2) {
        std::cout << "\n  Select Recipient Phone Number:" << std::endl;
        std::cout << "  1. Send to Registered Number (" << currentUser.phone
                  << ")" << std::endl;
        std::cout << "  2. Send to a Different WhatsApp Number" << std::endl;
        int phoneChoice = getValidInt("  Choose Option (1-2): ", 1, 2);

        std::string targetPhone = currentUser.phone;
        if (phoneChoice == 2) {
          targetPhone = getValidString("  Enter WhatsApp Number (with country "
                                       "code, e.g. +919876543210): ");
        }

        std::stringstream waMsg;
        waMsg << "*Aeroline Travels - Ticket Copy* 🎫\n\n"
              << "Dear " << currentUser.fullName << ",\n"
              << "Here is your ticket copy. Details:\n"
              << "• *Booking ID:* " << it->bookingId << "\n"
              << "• *Bus:* " << busObj.busName << " (" << busObj.busType
              << ")\n"
              << "• *Route:* " << busObj.source << " -> " << busObj.destination
              << "\n"
              << "• *Date:* " << it->date << " (" << busObj.depTime << ")\n"
              << "• *Seats:* ";
        for (size_t i = 0; i < it->seatNumbers.size(); ++i) {
          waMsg << it->seatNumbers[i]
                << (i + 1 < it->seatNumbers.size() ? ", " : "");
        }
        waMsg << "\n• *Total Fare:* Rs. " << std::fixed << std::setprecision(2)
              << it->farePaid << "\n\n"
              << "Your PDF printable copy has been created locally in your "
                 "workspace tickets directory.\n"
              << "Thank you for traveling with us! Safe journey! 🚌✨";

        std::string encodedMsg = urlEncode(waMsg.str());
        std::string waUrl =
            "whatsapp://send?phone=" + sanitizePhoneNumber(targetPhone) +
            "&text=" + encodedMsg;
        openBrowser(waUrl);
        std::cout
            << Color::GREEN
            << "  [✓] Launching installed WhatsApp Desktop app for number: "
            << targetPhone << Color::RESET << std::endl;

#ifdef _WIN32
        std::cout
            << Color::YELLOW
            << "  [!] Initializing automatic message transmission dispatch..."
            << Color::RESET << std::endl;
        std::cout << "      Launching background automation driver..."
                  << std::endl;
        std::thread(runWhatsAppAutomation).detach();
#endif
        std::cout << "\n  Press Enter to continue...";
        std::string dummy;
        std::getline(std::cin, dummy);
      }
    } else {
      std::cout << Color::RED << "  Booking ID not found." << Color::RESET
                << std::endl;
      std::cout << "\n  Press Enter to continue...";
      std::string dummy;
      std::getline(std::cin, dummy);
    }
  }

  void cancelBookingFlow() {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE << "\n  [ CANCEL TICKET BOOKING ]"
              << Color::RESET << std::endl;
    std::cout << "  =========================" << std::endl;

    std::vector<Booking> activeBookings;
    for (const auto &bk : bookings) {
      if (bk.username == currentUser.username && bk.status == "BOOKED") {
        activeBookings.push_back(bk);
      }
    }

    if (activeBookings.empty()) {
      std::cout << Color::RED << "  You have no active confirmed bookings."
                << Color::RESET << std::endl;
      std::cout << "\n  Press Enter to return to menu...";
      std::string dummy;
      std::getline(std::cin, dummy);
      return;
    }

    // Print list
    std::cout << "  " << std::string(70, '-') << std::endl;
    std::cout << "  " << std::left << std::setw(15) << "Booking ID"
              << std::setw(15) << "Bus ID" << std::setw(15) << "Date"
              << std::setw(15) << "Fare Paid" << std::endl;
    std::cout << "  " << std::string(70, '-') << std::endl;

    for (const auto &bk : activeBookings) {
      std::cout << "  " << std::left << std::setw(15) << bk.bookingId
                << std::setw(15) << bk.busId << std::setw(15) << bk.date
                << "Rs. " << std::fixed << std::setprecision(2) << bk.farePaid
                << std::endl;
    }
    std::cout << "  " << std::string(70, '-') << std::endl;

    std::string cancelId = toUpper(getValidString(
        "\n  Enter Booking ID to cancel (or 'BACK' to return): "));
    if (cancelId == "BACK")
      return;

    auto it =
        std::find_if(bookings.begin(), bookings.end(), [&](const Booking &bk) {
          return bk.bookingId == cancelId &&
                 bk.username == currentUser.username && bk.status == "BOOKED";
        });

    if (it != bookings.end()) {
      std::string choice =
          toUpper(getValidString("  Are you sure you want to cancel this "
                                 "booking? 10% penalty applies. (Y/N): "));
      if (choice == "Y") {
        it->status = "CANCELLED";
        double refund = it->farePaid * 0.90;

        showLoader("\n  Processing cancellation and releasing seats...", 8, 15);
        showLoader("  Calculating refund details...", 6, 12);

        saveBookings();

        std::cout << Color::GREEN << "\n  Ticket successfully cancelled!"
                  << Color::RESET << std::endl;
        std::cout << "  Original Fare Paid : Rs. " << std::fixed
                  << std::setprecision(2) << it->farePaid << std::endl;
        std::cout << "  Cancellation Charge (10%): Rs. "
                  << (it->farePaid * 0.10) << std::endl;
        std::cout << "  Refund Credited to original source: " << Color::GREEN
                  << "Rs. " << refund << Color::RESET << std::endl;
      } else {
        std::cout << Color::RED << "  Cancellation request aborted."
                  << Color::RESET << std::endl;
      }
    } else {
      std::cout << Color::RED << "  Invalid active Booking ID." << Color::RESET
                << std::endl;
    }

    std::cout << "\n  Press Enter to continue...";
    std::string dummy;
    std::getline(std::cin, dummy);
  }

  void viewCustomerProfile() {
    while (true) {
      clearScreen();
      showHeader();
      std::cout << Color::BOLD << Color::WHITE << "\n  [ USER PROFILE DETAILS ]"
                << Color::RESET << std::endl;
      std::cout << "  ========================" << std::endl;
      std::cout << "  Username   : " << currentUser.username << std::endl;
      std::cout << "  Full Name  : " << currentUser.fullName << std::endl;
      std::cout << "  Phone No.  : " << currentUser.phone << std::endl;
      std::cout << "  User Role  : " << currentUser.role << std::endl;
      std::cout << "  --------------------------------------------------------"
                << std::endl;
      std::cout << "  1. Update Registered Mobile Number" << std::endl;
      std::cout << "  2. Change Account Password" << std::endl;
      std::cout << "  3. Return to Dashboard" << std::endl;
      std::cout << "  --------------------------------------------------------"
                << std::endl;

      int choice = getValidInt("  Choose Option (1-3): ", 1, 3);
      if (choice == 1) {
        std::string newPhone =
            getValidString("  Enter New Mobile Number (with country code, e.g. "
                           "+919876543210): ");
        for (auto &u : users) {
          if (u.username == currentUser.username) {
            u.phone = newPhone;
            currentUser.phone = newPhone;
            break;
          }
        }
        saveUsers();
        std::cout << Color::GREEN << "\n  Mobile number updated successfully!"
                  << Color::RESET << std::endl;
        std::cout << "\n  Press Enter to continue...";
        std::string dummy;
        std::getline(std::cin, dummy);
      } else if (choice == 2) {
        std::string oldPass = getMaskedPassword("  Enter Current Password: ");
        if (hashPassword(oldPass) == currentUser.passwordHash) {
          std::string newPass = getMaskedPassword("  Enter New Password: ");
          std::string confirmPass =
              getMaskedPassword("  Confirm New Password: ");

          if (newPass == confirmPass) {
            for (auto &u : users) {
              if (u.username == currentUser.username) {
                u.passwordHash = hashPassword(newPass);
                currentUser.passwordHash = u.passwordHash;
                break;
              }
            }
            saveUsers();
            std::cout << Color::GREEN << "\n  Password changed successfully!"
                      << Color::RESET << std::endl;
          } else {
            std::cout << Color::RED << "\n  Error: New passwords do not match."
                      << Color::RESET << std::endl;
          }
        } else {
          std::cout << Color::RED << "\n  Error: Incorrect current password."
                    << Color::RESET << std::endl;
        }
        std::cout << "\n  Press Enter to continue...";
        std::string dummy;
        std::getline(std::cin, dummy);
      } else {
        break;
      }
    }
  }

  // ---------------------------------------------------------------
  // Customer - Request a New Bus Route
  // ---------------------------------------------------------------
  void requestRouteFlow() {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE
              << "\n  [ REQUEST A NEW BUS ROUTE ]" << Color::RESET << std::endl;
    std::cout << "  ===========================" << std::endl;
    std::cout << Color::CYAN
              << "  Can't find your desired route? Let us know and our admin\n"
              << "  team will review your request and add the route if feasible."
              << Color::RESET << std::endl << std::endl;

    // Show any existing requests by this user
    std::vector<RouteRequest> myRequests;
    for (const auto &rr : routeRequests) {
      if (rr.username == currentUser.username)
        myRequests.push_back(rr);
    }
    if (!myRequests.empty()) {
      std::cout << Color::BOLD << Color::YELLOW
                << "  Your Previous Route Requests:" << Color::RESET << std::endl;
      std::cout << "  " << std::string(72, '-') << std::endl;
      std::cout << "  " << std::left << std::setw(14) << "Request ID"
                << std::setw(14) << "From" << std::setw(14) << "To"
                << std::setw(12) << "Pref. Time" << "Status" << std::endl;
      std::cout << "  " << std::string(72, '-') << std::endl;
      for (const auto &rr : myRequests) {
        std::string statusCol;
        if (rr.status == "APPROVED") statusCol = Color::GREEN + rr.status + Color::RESET;
        else if (rr.status == "REJECTED") statusCol = Color::RED + rr.status + Color::RESET;
        else statusCol = Color::YELLOW + rr.status + Color::RESET;
        std::cout << "  " << std::left << std::setw(14) << rr.requestId
                  << std::setw(14) << rr.source << std::setw(14) << rr.destination
                  << std::setw(12) << rr.preferredTime << statusCol << std::endl;
      }
      std::cout << "  " << std::string(72, '-') << std::endl << std::endl;
    }

    std::string source = getValidString("  Origin City           : ");
    std::string destination = getValidString("  Destination City      : ");
    std::string prefTime = getValidString("  Preferred Departure Time (e.g. 08:00 AM): ");
    std::string notes = getValidString("  Additional Notes / Reason (or type 'none'): ");

    std::string confirm = toUpper(getValidString(
        "\n  Submit this route request? (Y/N): "));
    if (confirm != "Y") {
      std::cout << Color::YELLOW << "  Request submission cancelled."
                << Color::RESET << std::endl;
      std::cout << "\n  Press Enter to return...";
      std::string d; std::getline(std::cin, d);
      return;
    }

    // Generate a unique Request ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);
    std::string reqId = "RRQ-" + std::to_string(dis(gen));

    RouteRequest rr;
    rr.requestId    = reqId;
    rr.username     = currentUser.username;
    rr.source       = source;
    rr.destination  = destination;
    rr.preferredTime = prefTime;
    rr.notes        = notes;
    rr.status       = "PENDING";
    rr.timestamp    = getCurrentTimestamp();
    routeRequests.push_back(rr);
    saveRouteRequests();

    showLoader("\n  Submitting request to Aeroline admin panel...", 8, 15);
    std::cout << Color::GREEN
              << "  [✓] Route request submitted successfully!" << Color::RESET << std::endl;
    std::cout << "  Request ID : " << Color::BOLD << reqId << Color::RESET << std::endl;
    std::cout << "  Route      : " << source << " → " << destination << std::endl;
    std::cout << Color::CYAN
              << "  An administrator will review your request shortly."
              << Color::RESET << std::endl;
    std::cout << "\n  Press Enter to return to dashboard...";
    std::string d; std::getline(std::cin, d);
  }

  // -------------------------------------------------------------
  // Administrator Dashboard Menu
  // -------------------------------------------------------------

  void handleAdminMenu() {
    while (isLoggedIn) {
      clearScreen();
      showHeader();
      std::cout
          << Color::BRIGHT_BLACK
          << "  ┌────────────────────────────────────────────────────────┐"
          << Color::RESET << std::endl;
      std::stringstream title;
      title << "Welcome, " << currentUser.fullName << " (Administrator)";
      std::cout << "  " << Color::BRIGHT_BLACK << "│ " << Color::BOLD
                << Color::YELLOW << std::left << std::setw(55) << title.str()
                << Color::BRIGHT_BLACK << "│" << Color::RESET << std::endl;
      std::cout
          << Color::BRIGHT_BLACK
          << "  ├────────────────────────────────────────────────────────┤"
          << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "1. " << Color::RESET << std::left << std::setw(51)
                << "Add New Bus Schedule" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "2. " << Color::RESET << std::left << std::setw(51)
                << "View All Buses" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "3. " << Color::RESET << std::left << std::setw(51)
                << "Edit Bus Schedule" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "4. " << Color::RESET << std::left << std::setw(51)
                << "Delete Bus Schedule" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "5. " << Color::RESET << std::left << std::setw(51)
                << "View All Sales Bookings" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "6. " << Color::RESET << std::left << std::setw(51)
                << "View Statistics & Revenue Analytics" << Color::BRIGHT_BLACK
                << "│" << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "7. " << Color::RESET << std::left << std::setw(51)
                << "Manage Registered Users" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "8. " << Color::RESET << std::left << std::setw(51)
                << "Force Cancel a Booking" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "9. " << Color::RESET << std::left << std::setw(51)
                << "Refresh Online Bus Data" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "10." << Color::RESET << std::left << std::setw(51)
                << "Export All Bookings to CSV" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::LIGHT_CYAN
                << "11." << Color::RESET << std::left << std::setw(51)
                << "View & Approve Route Requests" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::RED << "12."
                << Color::RESET << std::left << std::setw(51)
                << "Clear All Booking History" << Color::BRIGHT_BLACK << "│"
                << Color::RESET << std::endl;
      std::cout << "  " << Color::BRIGHT_BLACK << "│  " << Color::RED << "13."
                << Color::RESET << std::left << std::setw(51) << "Logout"
                << Color::BRIGHT_BLACK << "│" << Color::RESET << std::endl;
      std::cout
          << Color::BRIGHT_BLACK
          << "  └────────────────────────────────────────────────────────┘"
          << Color::RESET << std::endl;
      std::cout << std::endl;

      int choice = getValidInt("  Choose Option (1-13): ", 1, 13);
      switch (choice) {
      case 1:
        addBusSchedule();
        break;
      case 2:
        viewBusesAdmin();
        break;
      case 3:
        editBusSchedule();
        break;
      case 4:
        deleteBusSchedule();
        break;
      case 5:
        viewAllBookingsAdmin();
        break;
      case 6:
        viewStatsAnalytics();
        break;
      case 7:
        manageUsersAdmin();
        break;
      case 8:
        forceCancelBookingAdmin();
        break;
      case 9:
        refreshOnlineBusData();
        break;
      case 10:
        exportBookingsCSV();
        break;
      case 11:
        viewAndApproveRouteRequests();
        break;
      case 12:
        clearAllBookingsFlow();
        break;
      case 13:
        handleLogout();
        break;
      }
    }
  }

  void addBusSchedule() {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE << "\n  [ ADD NEW BUS SCHEDULE ]"
              << Color::RESET << std::endl;
    std::cout << "  ========================" << std::endl;

    std::string id;
    while (true) {
      id = toUpper(getValidString("  Enter Unique Bus ID (e.g. BUS-108): "));
      auto it = std::find_if(buses.begin(), buses.end(),
                             [&](const Bus &b) { return b.busId == id; });
      if (it == buses.end())
        break;
      std::cout << Color::RED << "  Bus ID already exists. Enter a unique ID."
                << Color::RESET << std::endl;
    }

    std::string name = getValidString("  Enter Bus Name: ");

    std::string type;
    while (true) {
      std::cout << "  Select Bus Type:\n"
                << "    1. AC Seater\n"
                << "    2. Non-AC Seater\n"
                << "    3. AC Sleeper\n"
                << "    4. Non-AC Sleeper\n";
      int tChoice = getValidInt("  Enter Selection (1-4): ", 1, 4);
      if (tChoice == 1) {
        type = "AC Seater";
        break;
      } else if (tChoice == 2) {
        type = "Non-AC Seater";
        break;
      } else if (tChoice == 3) {
        type = "AC Sleeper";
        break;
      } else if (tChoice == 4) {
        type = "Non-AC Sleeper";
        break;
      }
    }

    std::string source = getValidString("  Origin City      : ");
    std::string destination = getValidString("  Destination City : ");
    std::string dep = getValidString("  Departure Time (e.g. 09:30 AM): ");
    std::string arr = getValidString("  Arrival Time (e.g. 05:00 PM): ");
    double fare = getValidDouble("  Fare (INR): ", 1.0, 50000.0);

    int seats;
    while (true) {
      seats = getValidInt(
          "  Total Seats (Must be a multiple of 4, Max 60): ", 8, 60);
      if (seats % 4 == 0)
        break;
      std::cout << Color::RED
                << "  Seats layout operates in rows of 4. Total seats must be "
                   "multiple of 4."
                << Color::RESET << std::endl;
    }

    Bus b = {id, name, type, source, destination, dep, arr, fare, seats};
    buses.push_back(b);
    saveBuses();

    showLoader("\n  Registering bus routing schema...", 8, 15);
    std::cout << Color::GREEN << "  Bus added successfully!" << Color::RESET
              << std::endl;
    std::cout << "\n  Press Enter to continue...";
    std::string dummy;
    std::getline(std::cin, dummy);
  }

  void viewBusesAdmin() const {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE
              << "\n  [ ALL REGISTERED ROUTE BUSES ]" << Color::RESET
              << std::endl;
    std::cout << "  ==============================" << std::endl;

    if (buses.empty()) {
      std::cout << Color::RED << "  No buses are registered in system."
                << Color::RESET << std::endl;
    } else {
      std::cout << "  " << std::string(90, '-') << std::endl;
      std::cout << "  " << std::left << std::setw(10) << "Bus ID"
                << std::setw(20) << "Bus Name" << std::setw(15) << "Type"
                << std::setw(12) << "From" << std::setw(12) << "To"
                << std::setw(10) << "Fare" << std::setw(8) << "Seats"
                << std::endl;
      std::cout << "  " << std::string(90, '-') << std::endl;

      for (const auto &b : buses) {
        std::cout << "  " << std::left << std::setw(10) << b.busId
                  << std::setw(20) << b.busName << std::setw(15) << b.busType
                  << std::setw(12) << b.source << std::setw(12) << b.destination
                  << "Rs. " << std::fixed << std::setprecision(2)
                  << std::setw(6) << b.fare << std::setw(8) << b.totalSeats
                  << std::endl;
      }
      std::cout << "  " << std::string(90, '-') << std::endl;
    }
    std::cout << "\n  Press Enter to return...";
    std::string dummy;
    std::getline(std::cin, dummy);
  }

  void editBusSchedule() {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE
              << "\n  [ EDIT BUS SCHEDULE DETAILS ]" << Color::RESET
              << std::endl;
    std::cout << "  =============================" << std::endl;

    std::string id = toUpper(getValidString("  Enter Bus ID to Edit: "));
    auto it = std::find_if(buses.begin(), buses.end(),
                           [&](const Bus &b) { return b.busId == id; });

    if (it != buses.end()) {
      std::cout << Color::CYAN
                << "  (Press Enter without typing to keep current value)\n"
                << Color::RESET << std::endl;

      std::cout << "  Current Name: " << it->busName << std::endl;
      std::cout << "  Enter New Name: ";
      std::string name;
      std::getline(std::cin, name);
      name = trim(name);
      if (!name.empty())
        it->busName = name;

      std::cout << "  Current Source: " << it->source << std::endl;
      std::cout << "  Enter New Source: ";
      std::string src;
      std::getline(std::cin, src);
      src = trim(src);
      if (!src.empty())
        it->source = src;

      std::cout << "  Current Destination: " << it->destination << std::endl;
      std::cout << "  Enter New Destination: ";
      std::string dest;
      std::getline(std::cin, dest);
      dest = trim(dest);
      if (!dest.empty())
        it->destination = dest;

      std::cout << "  Current Departure Time: " << it->depTime << std::endl;
      std::cout << "  Enter New Departure Time: ";
      std::string dep;
      std::getline(std::cin, dep);
      dep = trim(dep);
      if (!dep.empty())
        it->depTime = dep;

      std::cout << "  Current Arrival Time: " << it->arrTime << std::endl;
      std::cout << "  Enter New Arrival Time: ";
      std::string arr;
      std::getline(std::cin, arr);
      arr = trim(arr);
      if (!arr.empty())
        it->arrTime = arr;

      std::cout << "  Current Fare: Rs. " << std::fixed << std::setprecision(2)
                << it->fare << std::endl;
      std::cout << "  Enter New Fare (Or 0 to skip): ";
      std::string fareStr;
      std::getline(std::cin, fareStr);
      fareStr = trim(fareStr);
      if (!fareStr.empty()) {
        double f = std::stod(fareStr);
        if (f > 0)
          it->fare = f;
      }

      saveBuses();
      showLoader("\n  Updating schedule files...", 8, 15);
      std::cout << Color::GREEN << "  Bus schedule updated successfully!"
                << Color::RESET << std::endl;
    } else {
      std::cout << Color::RED << "  Bus ID not found." << Color::RESET
                << std::endl;
    }

    std::cout << "\n  Press Enter to continue...";
    std::string dummy;
    std::getline(std::cin, dummy);
  }

  void deleteBusSchedule() {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE
              << "\n  [ REMOVE BUS ROUTE SCHEDULE ]" << Color::RESET
              << std::endl;
    std::cout << "  =============================" << std::endl;

    std::string id = toUpper(getValidString("  Enter Bus ID to Delete: "));
    auto it = std::find_if(buses.begin(), buses.end(),
                           [&](const Bus &b) { return b.busId == id; });

    if (it != buses.end()) {
      std::cout << Color::RED << "  WARNING: Deleting Bus " << it->busName
                << " (" << it->busId
                << ") will make it unavailable for future reservations."
                << Color::RESET << std::endl;
      std::string confirm = toUpper(getValidString(
          "  Are you sure you want to delete this bus? (Y/N): "));
      if (confirm == "Y") {
        buses.erase(it);
        saveBuses();
        showLoader("\n  Purging route definitions...", 8, 15);
        std::cout << Color::GREEN << "  Bus deleted successfully!"
                  << Color::RESET << std::endl;
      } else {
        std::cout << Color::YELLOW << "  Deletion aborted." << Color::RESET
                  << std::endl;
      }
    } else {
      std::cout << Color::RED << "  Bus ID not found." << Color::RESET
                << std::endl;
    }

    std::cout << "\n  Press Enter to continue...";
    std::string dummy;
    std::getline(std::cin, dummy);
  }

  void viewAllBookingsAdmin() const {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE
              << "\n  [ SYSTEM-WIDE BOOKINGS SALES REPORT ]" << Color::RESET
              << std::endl;
    std::cout << "  =====================================" << std::endl;

    if (bookings.empty()) {
      std::cout << Color::RED
                << "  No bookings have been made yet in the system."
                << Color::RESET << std::endl;
    } else {
      std::cout << "  " << std::string(90, '-') << std::endl;
      std::cout << "  " << std::left << std::setw(12) << "Booking ID"
                << std::setw(12) << "Username" << std::setw(10) << "Bus ID"
                << std::setw(12) << "Travel Date" << std::setw(12)
                << "Fare Paid" << std::setw(12) << "Status"
                << "Timestamp" << std::endl;
      std::cout << "  " << std::string(90, '-') << std::endl;

      for (const auto &bk : bookings) {
        std::string statusStr = bk.status;
        if (bk.status == "BOOKED") {
          statusStr = Color::GREEN + "CONFIRMED" + Color::RESET;
        } else {
          statusStr = Color::RED + "CANCELLED" + Color::RESET;
        }
        std::cout << "  " << std::left << std::setw(12) << bk.bookingId
                  << std::setw(12) << bk.username << std::setw(10) << bk.busId
                  << std::setw(12) << bk.date << "Rs. " << std::fixed
                  << std::setprecision(2) << std::setw(8) << bk.farePaid
                  << std::setw(21) << statusStr << bk.timestamp << std::endl;
      }
      std::cout << "  " << std::string(90, '-') << std::endl;
    }

    std::cout << "\n  Press Enter to continue...";
    std::string dummy;
    std::getline(std::cin, dummy);
  }

  void viewStatsAnalytics() const {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE
              << "\n  [ PLATFORM METRICS & ANALYTICS ]" << Color::RESET
              << std::endl;
    std::cout << "  ================================" << std::endl;

    int totalUsers = users.size();
    int totalBuses = buses.size();
    int totalBookings = bookings.size();

    int activeBks = 0;
    int cancelledBks = 0;
    double grossRevenue = 0.0;
    double cancelledAmt = 0.0;

    for (const auto &bk : bookings) {
      if (bk.status == "BOOKED") {
        activeBks++;
        grossRevenue += bk.farePaid;
      } else {
        cancelledBks++;
        cancelledAmt += bk.farePaid;
      }
    }

    // Refund is 90%, penalty retained by admin is 10%
    double cancellationPenalties = cancelledAmt * 0.10;
    double netRevenue = grossRevenue + cancellationPenalties;

    std::cout << "  Total Registered Users     : " << totalUsers << std::endl;
    std::cout << "  Total Active Fleet Buses   : " << totalBuses << std::endl;
    std::cout << "  Total Transactions Logged  : " << totalBookings
              << std::endl;
    std::cout << "  --------------------------------------------------------"
              << std::endl;
    std::cout << "  Confirmed Reservations     : " << Color::GREEN << activeBks
              << Color::RESET << std::endl;
    std::cout << "  Cancelled Reservations     : " << Color::RED << cancelledBks
              << Color::RESET << std::endl;
    std::cout << "  --------------------------------------------------------"
              << std::endl;
    std::cout << "  Gross Ticket Revenue       : Rs. " << std::fixed
              << std::setprecision(2) << grossRevenue << std::endl;
    std::cout << "  Cancellation Penalties (10%): Rs. " << cancellationPenalties
              << std::endl;
    std::cout << "  Net Platform Revenue       : " << Color::GREEN << "Rs. "
              << netRevenue << Color::RESET << std::endl;
    std::cout << "  --------------------------------------------------------"
              << std::endl;

    // Occupancy calculation by Route
    std::cout << Color::BOLD << Color::CYAN << "\n  [ ROUTE UTILITY REPORT ]"
              << Color::RESET << std::endl;
    std::cout << "  " << std::string(60, '-') << std::endl;
    std::cout << "  " << std::left << std::setw(10) << "Bus ID" << std::setw(20)
              << "Route" << std::setw(15) << "Tickets Booked" << std::endl;
    std::cout << "  " << std::string(60, '-') << std::endl;

    for (const auto &b : buses) {
      int bookedSeats = 0;
      for (const auto &bk : bookings) {
        if (bk.busId == b.busId && bk.status == "BOOKED") {
          bookedSeats += bk.seatNumbers.size();
        }
      }
      std::string routeStr = b.source + "->" + b.destination;
      std::cout << "  " << std::left << std::setw(10) << b.busId
                << std::setw(20) << routeStr << std::setw(15) << bookedSeats
                << std::endl;
    }
    std::cout << "  " << std::string(60, '-') << std::endl;

    std::cout << "\n  Press Enter to continue...";
    std::string dummy;
    std::getline(std::cin, dummy);
  }

  void clearAllBookingsFlow() {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE << "\n  [ PURGE SYSTEM BOOKINGS ]"
              << Color::RESET << std::endl;
    std::cout << "  =========================" << std::endl;
    std::cout << Color::BOLD << Color::RED
              << "  WARNING: This will delete ALL ticket bookings permanently."
              << Color::RESET << std::endl;
    std::cout << "  This operation cannot be undone." << std::endl;
    std::cout << std::endl;
    std::string confirm = toUpper(getValidString(
        "  Are you sure you want to clear ALL bookings? (YES/NO): "));
    if (confirm == "YES") {
      bookings.clear();
      saveBookings();
      showLoader("\n  Purging transaction records...", 10, 15);
      std::cout << Color::GREEN << "  All bookings cleared successfully!"
                << Color::RESET << std::endl;
    } else {
      std::cout << Color::YELLOW
                << "\n  Operation cancelled. No bookings were cleared."
                << Color::RESET << std::endl;
    }
    std::cout << "\n  Press Enter to continue...";
    std::string dummy;
    std::getline(std::cin, dummy);
  }

  // ---------------------------------------------------------------
  // NEW: Customer - Reschedule Booking
  // ---------------------------------------------------------------
  void rescheduleBookingFlow() {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE << "\n  [ RESCHEDULE A BOOKING ]"
              << Color::RESET << std::endl;
    std::cout << "  ========================" << std::endl;

    std::vector<Booking *> activeBookings;
    for (auto &bk : bookings) {
      if (bk.username == currentUser.username && bk.status == "BOOKED") {
        activeBookings.push_back(&bk);
      }
    }

    if (activeBookings.empty()) {
      std::cout << Color::RED << "  No active bookings found to reschedule."
                << Color::RESET << std::endl;
      std::cout << "\n  Press Enter to return...";
      std::string dummy;
      std::getline(std::cin, dummy);
      return;
    }

    std::cout << "  Active Bookings:\n";
    std::cout << "  " << std::string(60, '-') << std::endl;
    std::cout << "  " << std::left << std::setw(15) << "Booking ID"
              << std::setw(12) << "Bus ID" << std::setw(14) << "Current Date"
              << "Fare" << std::endl;
    std::cout << "  " << std::string(60, '-') << std::endl;
    for (const auto *bk : activeBookings) {
      std::cout << "  " << std::left << std::setw(15) << bk->bookingId
                << std::setw(12) << bk->busId << std::setw(14) << bk->date
                << "Rs. " << std::fixed << std::setprecision(2) << bk->farePaid
                << std::endl;
    }
    std::cout << "  " << std::string(60, '-') << std::endl;

    std::string bkId = toUpper(getValidString(
        "\n  Enter Booking ID to Reschedule (or BACK to cancel): "));
    if (bkId == "BACK")
      return;

    Booking *target = nullptr;
    for (auto *bk : activeBookings) {
      if (bk->bookingId == bkId) {
        target = bk;
        break;
      }
    }

    if (!target) {
      std::cout << Color::RED
                << "  Booking ID not found in your active bookings."
                << Color::RESET << std::endl;
      std::cout << "\n  Press Enter to continue...";
      std::string d;
      std::getline(std::cin, d);
      return;
    }

    std::cout << Color::CYAN
              << "  Note: Rescheduling incurs a Rs. 50 change fee per ticket."
              << Color::RESET << std::endl;
    std::string newDate =
        getFutureDateInput("  Enter New Journey Date (YYYY-MM-DD): ");

    if (newDate == target->date) {
      std::cout << Color::YELLOW
                << "  New date is the same as current date. No changes made."
                << Color::RESET << std::endl;
      std::cout << "\n  Press Enter to continue...";
      std::string d;
      std::getline(std::cin, d);
      return;
    }

    std::string confirm =
        toUpper(getValidString("  Confirm reschedule from " + target->date +
                               " to " + newDate + "? (Y/N): "));
    if (confirm == "Y") {
      double fee = 50.0 * target->seatNumbers.size();
      target->date = newDate;
      target->farePaid += fee;
      target->timestamp = getCurrentTimestamp();
      saveBookings();
      showLoader("  Processing reschedule request...", 8, 15);
      std::cout << Color::GREEN << "\n  Booking successfully rescheduled to "
                << newDate << "!" << Color::RESET << std::endl;
      std::cout << "  Rescheduling fee charged: Rs. " << std::fixed
                << std::setprecision(2) << fee << std::endl;
    } else {
      std::cout << Color::YELLOW << "  Reschedule cancelled." << Color::RESET
                << std::endl;
    }
    std::cout << "\n  Press Enter to continue...";
    std::string d;
    std::getline(std::cin, d);
  }

  // ---------------------------------------------------------------
  // NEW: Customer - Travel Stats & Loyalty Points
  // ---------------------------------------------------------------
  void viewTravelStats() const {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE
              << "\n  [ MY TRAVEL STATS & LOYALTY POINTS ]" << Color::RESET
              << std::endl;
    std::cout << "  ====================================" << std::endl;

    int totalTrips = 0, confirmedTrips = 0, cancelledTrips = 0;
    double totalSpent = 0.0;
    std::set<std::string> citiesVisited;

    for (const auto &bk : bookings) {
      if (bk.username != currentUser.username)
        continue;
      totalTrips++;
      if (bk.status == "BOOKED") {
        confirmedTrips++;
        totalSpent += bk.farePaid;
        for (const auto &b : buses) {
          if (b.busId == bk.busId) {
            citiesVisited.insert(b.source);
            citiesVisited.insert(b.destination);
            break;
          }
        }
      } else {
        cancelledTrips++;
      }
    }

    // Loyalty: 1 point per Rs. 100 spent
    int loyaltyPoints = static_cast<int>(totalSpent / 100.0);
    std::string tier = "Bronze";
    if (loyaltyPoints >= 500)
      tier = "Platinum";
    else if (loyaltyPoints >= 200)
      tier = "Gold";
    else if (loyaltyPoints >= 100)
      tier = "Silver";

    std::cout << "  Traveller      : " << Color::BOLD << currentUser.fullName
              << Color::RESET << std::endl;
    std::cout << "  Member Since   : " << getCurrentDate() << std::endl;
    std::cout << "  --------------------------------------------------------"
              << std::endl;
    std::cout << "  Total Bookings : " << totalTrips << std::endl;
    std::cout << "  Confirmed Trips: " << Color::GREEN << confirmedTrips
              << Color::RESET << std::endl;
    std::cout << "  Cancelled Trips: " << Color::RED << cancelledTrips
              << Color::RESET << std::endl;
    std::cout << "  Cities Explored: " << Color::CYAN << citiesVisited.size()
              << Color::RESET << std::endl;
    std::cout << "  Total Spent    : " << Color::GREEN << "Rs. " << std::fixed
              << std::setprecision(2) << totalSpent << Color::RESET
              << std::endl;
    std::cout << "  --------------------------------------------------------"
              << std::endl;
    std::cout << "  Loyalty Points : " << Color::YELLOW << loyaltyPoints
              << " pts" << Color::RESET << std::endl;
    std::cout << "  Member Tier    : " << Color::BOLD;
    if (tier == "Platinum")
      std::cout << Color::LIGHT_CYAN;
    else if (tier == "Gold")
      std::cout << Color::YELLOW;
    else if (tier == "Silver")
      std::cout << Color::WHITE;
    else
      std::cout << Color::BRIGHT_BLACK;
    std::cout << "  ★ " << tier << " Member" << Color::RESET << std::endl;
    std::cout << "  --------------------------------------------------------"
              << std::endl;

    if (!citiesVisited.empty()) {
      std::cout << "  Cities Visited : ";
      bool first = true;
      for (const auto &c : citiesVisited) {
        if (!first)
          std::cout << ", ";
        std::cout << c;
        first = false;
      }
      std::cout << std::endl;
    }

    std::cout << "\n  Press Enter to return to dashboard...";
    std::string dummy;
    std::getline(std::cin, dummy);
  }

  // ---------------------------------------------------------------
  // NEW: Admin - Manage Registered Users
  // ---------------------------------------------------------------
  void manageUsersAdmin() {
    while (true) {
      clearScreen();
      showHeader();
      std::cout << Color::BOLD << Color::WHITE
                << "\n  [ MANAGE REGISTERED USERS ]" << Color::RESET
                << std::endl;
      std::cout << "  ===========================" << std::endl;
      std::cout << "  " << std::string(70, '-') << std::endl;
      std::cout << "  " << std::left << std::setw(15) << "Username"
                << std::setw(22) << "Full Name" << std::setw(18) << "Phone"
                << "Role" << std::endl;
      std::cout << "  " << std::string(70, '-') << std::endl;
      for (const auto &u : users) {
        std::string roleColour =
            (u.role == "ADMIN") ? Color::MAGENTA : Color::LIGHT_BLUE;
        std::cout << "  " << std::left << std::setw(15) << u.username
                  << std::setw(22) << u.fullName << std::setw(18) << u.phone
                  << roleColour << u.role << Color::RESET << std::endl;
      }
      std::cout << "  " << std::string(70, '-') << std::endl;
      std::cout << "\n  Options:" << std::endl;
      std::cout << "  1. Delete a User Account" << std::endl;
      std::cout << "  2. Reset a User's Password" << std::endl;
      std::cout << "  3. Back to Admin Panel" << std::endl;

      int choice = getValidInt("  Choose Option (1-3): ", 1, 3);
      if (choice == 3)
        break;

      std::string targetUser =
          toLower(getValidString("  Enter Username to Manage: "));
      if (targetUser == currentUser.username) {
        std::cout << Color::RED << "  Cannot modify your own account from here."
                  << Color::RESET << std::endl;
        std::cout << "\n  Press Enter to continue...";
        std::string d;
        std::getline(std::cin, d);
        continue;
      }

      auto it = std::find_if(users.begin(), users.end(), [&](const User &u) {
        return u.username == targetUser;
      });
      if (it == users.end()) {
        std::cout << Color::RED << "  User not found." << Color::RESET
                  << std::endl;
        std::cout << "\n  Press Enter to continue...";
        std::string d;
        std::getline(std::cin, d);
        continue;
      }

      if (choice == 1) {
        std::string confirm =
            toUpper(getValidString("  Delete account '" + targetUser +
                                   "'? This is permanent. (Y/N): "));
        if (confirm == "Y") {
          users.erase(it);
          saveUsers();
          std::cout << Color::GREEN << "  User '" << targetUser
                    << "' deleted successfully." << Color::RESET << std::endl;
        } else {
          std::cout << Color::YELLOW << "  Deletion cancelled." << Color::RESET
                    << std::endl;
        }
      } else if (choice == 2) {
        std::string newPass = getMaskedPassword("  Enter New Password for '" +
                                                targetUser + "': ");
        it->passwordHash = hashPassword(newPass);
        saveUsers();
        std::cout << Color::GREEN << "  Password for '" << targetUser
                  << "' reset successfully." << Color::RESET << std::endl;
      }
      std::cout << "\n  Press Enter to continue...";
      std::string d;
      std::getline(std::cin, d);
    }
  }

  // ---------------------------------------------------------------
  // NEW: Admin - Force Cancel Any Booking
  // ---------------------------------------------------------------
  void forceCancelBookingAdmin() {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE
              << "\n  [ FORCE CANCEL A BOOKING (ADMIN) ]" << Color::RESET
              << std::endl;
    std::cout << "  ==================================" << std::endl;

    std::vector<Booking *> activeBookings;
    for (auto &bk : bookings) {
      if (bk.status == "BOOKED")
        activeBookings.push_back(&bk);
    }

    if (activeBookings.empty()) {
      std::cout << Color::RED << "  No active bookings in the system."
                << Color::RESET << std::endl;
      std::cout << "\n  Press Enter to return...";
      std::string d;
      std::getline(std::cin, d);
      return;
    }

    std::cout << "  Active Bookings in System:\n";
    std::cout << "  " << std::string(72, '-') << std::endl;
    std::cout << "  " << std::left << std::setw(14) << "Booking ID"
              << std::setw(14) << "Username" << std::setw(10) << "Bus ID"
              << std::setw(13) << "Date" << "Fare" << std::endl;
    std::cout << "  " << std::string(72, '-') << std::endl;
    for (const auto *bk : activeBookings) {
      std::cout << "  " << std::left << std::setw(14) << bk->bookingId
                << std::setw(14) << bk->username << std::setw(10) << bk->busId
                << std::setw(13) << bk->date << "Rs. " << std::fixed
                << std::setprecision(2) << bk->farePaid << std::endl;
    }
    std::cout << "  " << std::string(72, '-') << std::endl;

    std::string bkId = toUpper(
        getValidString("\n  Enter Booking ID to Force Cancel (or BACK): "));
    if (bkId == "BACK")
      return;

    Booking *target = nullptr;
    for (auto *bk : activeBookings) {
      if (bk->bookingId == bkId) {
        target = bk;
        break;
      }
    }

    if (!target) {
      std::cout << Color::RED << "  Booking ID not found among active bookings."
                << Color::RESET << std::endl;
    } else {
      std::string confirm = toUpper(
          getValidString("  Force cancel booking '" + bkId + "' for user '" +
                         target->username + "'? (Y/N): "));
      if (confirm == "Y") {
        target->status = "CANCELLED";
        saveBookings();
        showLoader("  Processing admin cancellation...", 8, 15);
        std::cout << Color::GREEN << "  Booking '" << bkId
                  << "' cancelled by admin." << Color::RESET << std::endl;
      } else {
        std::cout << Color::YELLOW << "  Force cancel aborted." << Color::RESET
                  << std::endl;
      }
    }
    std::cout << "\n  Press Enter to continue...";
    std::string d;
    std::getline(std::cin, d);
  }

  // ---------------------------------------------------------------
  // NEW: Admin - Refresh Online Bus Data
  // ---------------------------------------------------------------
  void refreshOnlineBusData() {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE
              << "\n  [ REFRESH ONLINE BUS DATA ]" << Color::RESET << std::endl;
    std::cout << "  ===========================" << std::endl;
    std::cout
        << Color::YELLOW
        << "  WARNING: This will replace the current bus schedule with the"
        << "\n  latest data from the online server. Existing bookings are "
           "unaffected."
        << Color::RESET << std::endl;
    std::string confirm = toUpper(
        getValidString("  Proceed with data refresh from internet? (Y/N): "));
    if (confirm != "Y") {
      std::cout << Color::YELLOW << "  Refresh cancelled." << Color::RESET
                << std::endl;
      std::cout << "\n  Press Enter to continue...";
      std::string d;
      std::getline(std::cin, d);
      return;
    }

    // Delete local buses.txt so loadBuses() fetches fresh from online
    std::remove(busesFile.c_str());
    buses.clear();
    std::cout << Color::CYAN << "\n  Connecting to Aeroline central server..."
              << Color::RESET << std::endl;
    loadBuses();

    if (!buses.empty()) {
      std::cout << Color::GREEN << "  [✓] Bus data refreshed successfully! "
                << buses.size() << " routes loaded." << Color::RESET
                << std::endl;
    } else {
      std::cout
          << Color::RED
          << "  [!] Could not refresh bus data. Check internet connection."
          << Color::RESET << std::endl;
    }
    std::cout << "\n  Press Enter to continue...";
    std::string d;
    std::getline(std::cin, d);
  }

  // ---------------------------------------------------------------
  // NEW: Admin - Export All Bookings to CSV
  // ---------------------------------------------------------------
  void exportBookingsCSV() {
    clearScreen();
    showHeader();
    std::cout << Color::BOLD << Color::WHITE << "\n  [ EXPORT BOOKINGS TO CSV ]"
              << Color::RESET << std::endl;
    std::cout << "  ==========================" << std::endl;

    std::string filename = "bookings_export_" + getCurrentDate() + ".csv";
    std::ofstream csv(filename);
    if (!csv.is_open()) {
      std::cout << Color::RED << "  Failed to create CSV file." << Color::RESET
                << std::endl;
      std::cout << "\n  Press Enter to continue...";
      std::string d;
      std::getline(std::cin, d);
      return;
    }

    // Write CSV header
    csv << "Booking ID,Username,Bus ID,Travel Date,Seats,Passengers,Fare "
           "Paid,Status,Timestamp\n";
    for (const auto &bk : bookings) {
      // Seats
      std::string seats;
      for (size_t i = 0; i < bk.seatNumbers.size(); ++i)
        seats += std::to_string(bk.seatNumbers[i]) +
                 (i + 1 < bk.seatNumbers.size() ? ";" : "");
      // Passengers
      std::string passengers;
      for (size_t i = 0; i < bk.passengers.size(); ++i)
        passengers += bk.passengers[i].name + "(" +
                      std::to_string(bk.passengers[i].age) + "/" +
                      bk.passengers[i].gender + ")" +
                      (i + 1 < bk.passengers.size() ? ";" : "");
      csv << bk.bookingId << "," << bk.username << "," << bk.busId << ","
          << bk.date << ","
          << "\"" << seats << "\","
          << "\"" << passengers << "\"," << std::fixed << std::setprecision(2)
          << bk.farePaid << "," << bk.status << "," << bk.timestamp << "\n";
    }
    csv.close();

    showLoader("  Generating CSV export file...", 8, 15);
    std::cout << Color::GREEN
              << "  [✓] Export successful! File saved as: " << Color::BOLD
              << filename << Color::RESET << std::endl;
    std::cout << "      Total records exported: " << bookings.size()
              << std::endl;
    std::cout << "\n  Press Enter to continue...";
    std::string d;
    std::getline(std::cin, d);
  }
  // ---------------------------------------------------------------
  // Admin - View & Approve Customer Route Requests
  // ---------------------------------------------------------------
  void viewAndApproveRouteRequests() {
    while (true) {
      clearScreen();
      showHeader();
      std::cout << Color::BOLD << Color::WHITE
                << "\n  [ CUSTOMER ROUTE REQUESTS ]" << Color::RESET << std::endl;
      std::cout << "  ===========================" << std::endl;

      // Separate pending from resolved
      std::vector<int> pendingIdx, resolvedIdx;
      for (int i = 0; i < (int)routeRequests.size(); ++i) {
        if (routeRequests[i].status == "PENDING")
          pendingIdx.push_back(i);
        else
          resolvedIdx.push_back(i);
      }

      // --- PENDING REQUESTS TABLE ---
      std::cout << Color::BOLD << Color::YELLOW
                << "\n  Pending Requests (" << pendingIdx.size() << "):"
                << Color::RESET << std::endl;
      if (pendingIdx.empty()) {
        std::cout << Color::BRIGHT_BLACK
                  << "  No pending route requests." << Color::RESET << std::endl;
      } else {
        std::cout << "  " << std::string(80, '-') << std::endl;
        std::cout << "  " << std::left
                  << std::setw(12) << "Req ID"
                  << std::setw(12) << "Username"
                  << std::setw(14) << "From"
                  << std::setw(14) << "To"
                  << std::setw(12) << "Pref. Time"
                  << "Notes" << std::endl;
        std::cout << "  " << std::string(80, '-') << std::endl;
        for (int idx : pendingIdx) {
          const auto &rr = routeRequests[idx];
          std::string notesTrunc = rr.notes.length() > 18
                                   ? rr.notes.substr(0, 15) + "..."
                                   : rr.notes;
          std::cout << "  " << std::left
                    << std::setw(12) << rr.requestId
                    << std::setw(12) << rr.username
                    << std::setw(14) << rr.source
                    << std::setw(14) << rr.destination
                    << std::setw(12) << rr.preferredTime
                    << notesTrunc << std::endl;
        }
        std::cout << "  " << std::string(80, '-') << std::endl;
      }

      // --- RESOLVED REQUESTS (last 5) ---
      if (!resolvedIdx.empty()) {
        std::cout << Color::BOLD << Color::BRIGHT_BLACK
                  << "\n  Recently Resolved Requests (last "
                  << std::min((int)resolvedIdx.size(), 5) << "):"
                  << Color::RESET << std::endl;
        std::cout << "  " << std::string(70, '-') << std::endl;
        int showFrom = std::max(0, (int)resolvedIdx.size() - 5);
        for (int k = showFrom; k < (int)resolvedIdx.size(); ++k) {
          const auto &rr = routeRequests[resolvedIdx[k]];
          std::string statusCol = (rr.status == "APPROVED")
              ? Color::GREEN + rr.status + Color::RESET
              : Color::RED   + rr.status + Color::RESET;
          std::cout << "  " << std::left
                    << std::setw(12) << rr.requestId
                    << std::setw(12) << rr.username
                    << rr.source << " → " << rr.destination
                    << "  [" << statusCol << "]" << std::endl;
        }
        std::cout << "  " << std::string(70, '-') << std::endl;
      }

      std::cout << std::endl;
      std::cout << "  Options:" << std::endl;
      std::cout << "  1. Review & Act on a Pending Request" << std::endl;
      std::cout << "  2. Back to Admin Panel" << std::endl;

      int choice = getValidInt("  Choose Option (1-2): ", 1, 2);
      if (choice == 2)
        break;

      if (pendingIdx.empty()) {
        std::cout << Color::YELLOW
                  << "  No pending requests to review."
                  << Color::RESET << std::endl;
        std::cout << "\n  Press Enter to continue...";
        std::string d; std::getline(std::cin, d);
        continue;
      }

      std::string reqId = toUpper(
          getValidString("  Enter Request ID to Review (or BACK): "));
      if (reqId == "BACK")
        continue;

      // Find the request
      RouteRequest *target = nullptr;
      for (auto &rr : routeRequests) {
        if (rr.requestId == reqId && rr.status == "PENDING") {
          target = &rr;
          break;
        }
      }

      if (!target) {
        std::cout << Color::RED
                  << "  Request ID not found among pending requests."
                  << Color::RESET << std::endl;
        std::cout << "\n  Press Enter to continue...";
        std::string d; std::getline(std::cin, d);
        continue;
      }

      // Show full request details
      clearScreen();
      showHeader();
      std::cout << Color::BOLD << Color::WHITE
                << "\n  [ ROUTE REQUEST DETAILS ]" << Color::RESET << std::endl;
      std::cout << "  =========================" << std::endl;
      std::cout << "  Request ID   : " << Color::BOLD << target->requestId
                << Color::RESET << std::endl;
      std::cout << "  Submitted by : " << target->username << std::endl;
      std::cout << "  Route        : " << Color::CYAN << target->source
                << " → " << target->destination << Color::RESET << std::endl;
      std::cout << "  Pref. Time   : " << target->preferredTime << std::endl;
      std::cout << "  Notes        : " << target->notes << std::endl;
      std::cout << "  Submitted at : " << target->timestamp << std::endl;
      std::cout << "  --------------------------------------------------------"
                << std::endl;
      std::cout << "\n  Admin Actions:" << std::endl;
      std::cout << "  1. " << Color::GREEN << "Approve & Add Bus Route to System"
                << Color::RESET << std::endl;
      std::cout << "  2. " << Color::RED   << "Reject Request"
                << Color::RESET << std::endl;
      std::cout << "  3. Skip (Keep Pending)" << std::endl;

      int act = getValidInt("  Choose Action (1-3): ", 1, 3);

      if (act == 3) {
        continue;
      } else if (act == 2) {
        // Reject
        std::string reason = getValidString("  Enter reason for rejection (internal note): ");
        (void)reason; // stored conceptually; not persisted separately
        target->status = "REJECTED";
        saveRouteRequests();
        showLoader("  Processing rejection...", 6, 12);
        std::cout << Color::RED
                  << "  Request '" << reqId << "' marked as REJECTED."
                  << Color::RESET << std::endl;
        std::cout << "\n  Press Enter to continue...";
        std::string d; std::getline(std::cin, d);
      } else if (act == 1) {
        // Approve: launch addBusSchedule-style flow, pre-filled with request data
        clearScreen();
        showHeader();
        std::cout << Color::BOLD << Color::WHITE
                  << "\n  [ ADD BUS SCHEDULE — From Route Request ]" << Color::RESET << std::endl;
        std::cout << "  =========================================" << std::endl;
        std::cout << Color::CYAN
                  << "  Pre-filling details from request '" << reqId << "'."
                  << "\n  You can modify any field before confirming."
                  << Color::RESET << std::endl << std::endl;

        // Bus ID
        std::string id;
        while (true) {
          id = toUpper(getValidString("  Enter Unique Bus ID (e.g. BUS-210): "));
          auto it = std::find_if(buses.begin(), buses.end(),
                                 [&](const Bus &b) { return b.busId == id; });
          if (it == buses.end()) break;
          std::cout << Color::RED << "  Bus ID already exists. Enter a unique ID."
                    << Color::RESET << std::endl;
        }

        std::string name = getValidString("  Enter Bus Name: ");

        // Bus Type
        std::string type;
        while (true) {
          std::cout << "  Select Bus Type:\n"
                    << "    1. AC Seater\n"
                    << "    2. Non-AC Seater\n"
                    << "    3. AC Sleeper\n"
                    << "    4. Non-AC Sleeper\n";
          int tChoice = getValidInt("  Enter Selection (1-4): ", 1, 4);
          if      (tChoice == 1) { type = "AC Seater";     break; }
          else if (tChoice == 2) { type = "Non-AC Seater"; break; }
          else if (tChoice == 3) { type = "AC Sleeper";    break; }
          else if (tChoice == 4) { type = "Non-AC Sleeper"; break; }
        }

        // Pre-fill source/destination from request, but allow override
        std::cout << "  Origin City (requested: " << Color::CYAN
                  << target->source << Color::RESET << ") : ";
        std::string source;
        std::getline(std::cin, source);
        source = trim(source);
        if (source.empty()) source = target->source;

        std::cout << "  Destination City (requested: " << Color::CYAN
                  << target->destination << Color::RESET << ") : ";
        std::string destination;
        std::getline(std::cin, destination);
        destination = trim(destination);
        if (destination.empty()) destination = target->destination;

        // Pre-fill preferred time
        std::cout << "  Departure Time (requested: " << Color::CYAN
                  << target->preferredTime << Color::RESET << ") : ";
        std::string dep;
        std::getline(std::cin, dep);
        dep = trim(dep);
        if (dep.empty()) dep = target->preferredTime;

        std::string arr = getValidString("  Arrival Time (e.g. 05:00 PM): ");
        double fare = getValidDouble("  Fare (INR): ", 1.0, 50000.0);

        int seats;
        while (true) {
          seats = getValidInt(
              "  Total Seats (multiple of 4, Max 60): ", 8, 60);
          if (seats % 4 == 0) break;
          std::cout << Color::RED
                    << "  Total seats must be a multiple of 4."
                    << Color::RESET << std::endl;
        }

        Bus b = {id, name, type, source, destination, dep, arr, fare, seats};
        buses.push_back(b);
        saveBuses();

        // Mark request approved
        target->status = "APPROVED";
        saveRouteRequests();

        showLoader("\n  Registering bus route and approving request...", 10, 15);
        std::cout << Color::GREEN
                  << "  [✓] Bus '" << name << "' added to the system!"
                  << Color::RESET << std::endl;
        std::cout << Color::GREEN
                  << "  [✓] Route request '" << reqId << "' marked as APPROVED."
                  << Color::RESET << std::endl;
        std::cout << "  Route: " << source << " → " << destination
                  << " | Dep: " << dep << " | Fare: Rs. "
                  << std::fixed << std::setprecision(2) << fare << std::endl;
        std::cout << "\n  Press Enter to continue...";
        std::string d; std::getline(std::cin, d);
      }
    }
  }
};

// -------------------------------------------------------------
// Entry Point
// -------------------------------------------------------------

int main() {
  // Setup terminal and seed data
  BusBookingSystem system;
  system.handleStartMenu();
  return 0;
}
