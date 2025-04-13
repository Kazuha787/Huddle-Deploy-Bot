# KAZUHA's Huddle Deploy Bot 🚀

Welcome to **KAZUHA's Huddle Token Deployer**, a fully automated, multi-wallet ERC-20 deployment bot built for the **Huddle Testnet**. This script features a sleek terminal UI using `blessed`, random token name generation, and automatic transfers to addresses — all wrapped up in a 24-hour deploy loop.

![Deploy like a boss](https://img.shields.io/badge/Built%20With-Node.js-green)
![Huddle Testnet](https://img.shields.io/badge/Network-Huddle-blue)
![Made by Kazuha](https://img.shields.io/badge/Made%20By-Kazuha-ff69b4)


# Telegram Channel: .[Channel](https://t.me/Offical_Im_kazuha)
# GitHub Repository: [HUDDLE-BOT](https://github.com/Kazuha787/Huddle-Deploy-Bot.git)
---

## Features ✨

- **Multi-wallet support** via `.env` private keys  
- **ERC-20 contract auto deployment** (Solidity)  
- **Auto transfer** tokens to random addresses  
- **Live terminal UI** with real-time logs and status  
- **Random name + symbol** generator  
- **24h loop** — set it and forget it  
- Error handling & smart logs  
- Built with `ethers.js`, `solc`, `blessed`, `dotenv`

---

## Preview 📸

![computer-coding-gif-5](https://github.com/user-attachments/assets/346ca33e-b44b-49fa-b338-2968e848428e)


---

## Installation ⚙️

```bash
git clone https://github.com/Kazuha787/Huddle-Deploy-Bot.git
cd Huddle-Deploy-Bot
```
# Install The Depency 
```
npm install
```
# Setup 🛠️

1. Create `.env` file in root:
```
PRIVATE_KEYS=yourPrivateKey1,yourPrivateKey2,...
```

2. Add token contract

Make sure token.sol (your ERC-20 contract) is in the root directory.

4. Add target addresses
Create addresses.txt and paste wallet addresses (one per line) to receive token transfers.
---
## File Structure 🗂️

```bash
Huddle-Deploy-Bot/
├── addresses.txt            # Wallets to send deployed tokens to
├── token.sol                # Your ERC-20 contract (Solidity)
├── index.js                 # Main deployment script
├── utils.js                 # Helper functions (compile, deploy, etc.)
├── preview.png              # (Optional) Terminal UI screenshot
├── .env                     # Environment variables (PRIVATE_KEYS)
├── .gitignore               # Node modules & .env ignored
├── package.json             # Dependencies and scripts
└── README.md                # This sexy file
```

# Run the Bot ⚡
```
npm start
```
Tech Stack 🧠
Node.js
Ethers.js
Solidity + solc
Blessed (for TUI)
dotenv
fs / path / timers
---

Author & Credits

Made with love by Kazuha
GitHub: Kazuha787
---
License

MIT — feel free to use, modify, and deploy your tokens like a pro.
---
Star the repo if you liked it!
Let's dominate testnets together.
---
Let me know if you want me to add a logo, preview GIF, or even auto-deploy GitHub Actions.

