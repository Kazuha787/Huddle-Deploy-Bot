import { ethers } from "ethers";
import * as dotenv from "dotenv";
import solc from "solc";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { generateUniqueNames } from "./utils/randomName.js";
import { setTimeout as wait } from "timers/promises";
import blessed from "blessed";

dotenv.config();
const __dirname = path.dirname(fileURLToPath(import.meta.url));

const provider = new ethers.JsonRpcProvider("https://huddle-testnet.rpc.caldera.xyz/http");
const privateKeys = process.env.PRIVATE_KEYS.split(",");
const wallets = privateKeys.map((key) => new ethers.Wallet(key, provider));

const tokenSource = fs.readFileSync(path.join(__dirname, "token.sol"), "utf8");
const addresses = fs.readFileSync("addresses.txt", "utf-8").split("\n").filter(Boolean);

// Initialize Blessed UI
const screen = blessed.screen({
  smartCSR: true,
  title: "KAZUHA's Token Deployer",
});

// Create header with KAZUHA branding
const header = blessed.box({
  top: 0,
  left: "center",
  width: "100%",
  height: 3,
  content: "{bold}{green-fg}KAZUHA's Huddle Testnet Deploy{/}",
  tags: true,
  border: { type: "line", fg: "green" },
  style: { fg: "green", bold: true },
});

// Create log box for deployment logs
const logBox = blessed.log({
  top: 3,
  left: 0,
  width: "70%",
  height: "100%-5",
  label: " {bold}Deployment Logs{/} ",
  border: { type: "line", fg: "green" },
  style: { fg: "white", bg: "black" },
  scrollable: true,
  scrollbar: { bg: "green" },
  tags: true,
});

// Create status box for wallet and progress info
const statusBox = blessed.box({
  top: 3,
  right: 0,
  width: "30%",
  height: "100%-5",
  label: " {bold}System Status{/} ",
  border: { type: "line", fg: "green" },
  style: { fg: "white", bg: "black" },
  content: "",
  tags: true,
});

// Create footer for controls
const footer = blessed.box({
  bottom: 0,
  left: 0,
  width: "100%",
  height: 2,
  content: "{yellow-fg}Press Q or ESC to exit{/}",
  tags: true,
  style: { fg: "yellow", bg: "black" },
});

// Append elements to screen
screen.append(header);
screen.append(logBox);
screen.append(statusBox);
screen.append(footer);

// Handle keypresses
screen.key(["escape", "q", "C-c"], () => process.exit(0));

// Custom logging function to update UI
function log(message) {
  logBox.add(`{white-fg}${new Date().toISOString()} | ${message}{/}`);
  screen.render();
}

function updateStatus() {
  let content = `{bold}{cyan-fg}Wallet Status:{/}\n`;
  wallets.forEach((wallet, index) => {
    content += `Wallet ${index + 1}: {green-fg}${wallet.address.slice(0, 8)}...{/}\n`;
  });
  content += `\n{bold}{cyan-fg}Next Run:{/} Calculating...`;
  statusBox.setContent(content);
  screen.render();
}

// Compile contract
function compileContract() {
  const input = {
    language: "Solidity",
    sources: { "Token.sol": { content: tokenSource } },
    settings: {
      outputSelection: { "*": { "*": ["abi", "evm.bytecode.object"] } },
    },
  };
  const output = JSON.parse(solc.compile(JSON.stringify(input)));
  const contract = output.contracts["Token.sol"]["Token"];
  return { abi: contract.abi, bytecode: contract.evm.bytecode.object };
}

// Deploy token
async function deployToken(wallet, name, symbol) {
  const { abi, bytecode } = compileContract();
  const factory = new ethers.ContractFactory(abi, bytecode, wallet);
  const initialSupply = ethers.parseUnits("1000", 18);

  const balance = await provider.getBalance(wallet.address);
  log(`{yellow-fg}💰 Wallet (${wallet.address.slice(0, 8)}...) balance: ${ethers.formatEther(balance)} ETH{/}`);
  if (balance < ethers.parseEther("0.000001")) {
    throw new Error("⚠️ Balance too low to deploy contract.");
  }

  log(`{cyan-fg}🚀 Deploying token ${name.toUpperCase()} (${symbol})...{/}`);

  let contract;
  try {
    contract = await factory.deploy(name, symbol, initialSupply);
  } catch (err) {
    throw new Error(`❌ Deploy error: ${err.message}`);
  }

  const txHash = contract.deploymentTransaction().hash;
  log(`{blue-fg}📡 Awaiting deployment, tx hash: ${txHash.slice(0, 8)}...{/}`);

  try {
    await contract.waitForDeployment();
  } catch (err) {
    throw new Error(`❌ waitForDeployment error: ${err.message}`);
  }

  const deployedAddress = await contract.getAddress();
  log(`{green-fg}✅ Deployed at: ${deployedAddress.slice(0, 8)}...{/}`);

  return new ethers.Contract(deployedAddress, abi, wallet);
}

// Send tokens to random addresses
async function sendToRandomAddresses(contract) {
  const targets = [...addresses].sort(() => 0.5 - Math.random()).slice(0, 7);
  for (const to of targets) {
    const amount = ethers.parseUnits((Math.floor(Math.random() * 50) + 1).toString(), 18);
    try {
      const tx = await contract.transfer(to, amount);
      await tx.wait();
      log(`{green-fg}🔁 Sent ${ethers.formatUnits(amount)} tokens to ${to.slice(0, 8)}...{/}`);
    } catch (err) {
      log(`{red-fg}❌ Error sending to ${to.slice(0, 8)}...: ${err.message}{/}`);
    }
  }
}

// Main loop with UI integration
async function mainLoop() {
  updateStatus();
  while (true) {
    const now = new Date();
    log(`{bold}{magenta-fg}📦 Deploying 5 contract(s) per wallet at ${now.toUTCString()}{/}`);

    for (const wallet of wallets) {
      log(`{cyan-fg}👛 Using wallet: ${wallet.address.slice(0, 8)}...{/}`);
      const names = generateUniqueNames(5);
      log(`{green-fg}✅ Generated names: ${names.map(n => n.name).join(", ")}{/}`);

      for (let i = 0; i < names.length; i++) {
        const { name, symbol } = names[i];
        try {
          const contract = await deployToken(wallet, name, symbol);
          log(`{green-fg}[${i + 1}] ✅ Deployed ${name.toUpperCase()} (${symbol}) at ${contract.target.slice(0, 8)}...{/}`);
          await sendToRandomAddresses(contract);
        } catch (err) {
          log(`{red-fg}❌ Failed to deploy ${name}: ${err.message}{/}`);
        }
        await wait(3000); // 3 seconds between deployments
      }
    }

    const nextRun = new Date(now.getTime() + 24 * 60 * 60 * 1000);
    while (new Date() < nextRun) {
      const remaining = new Date(nextRun - new Date());
      const str = remaining.toISOString().substr(11, 8);
      statusBox.setContent(
        `{bold}{cyan-fg}Wallet Status:{/}\n` +
        wallets.map((w, i) => `Wallet ${i + 1}: {green-fg}${w.address.slice(0, 8)}...{/}`).join("\n") +
        `\n\n{bold}{cyan-fg}Next Run:{/} {yellow-fg}${str}{/}`
      );
      screen.render();
      await wait(1000);
    }
  }
}

// Start the loop
mainLoop().catch(err => {
  log(`{red-fg}❌ Fatal error: ${err.message}{/}`);
  process.exit(1);
});

// Render the screen initially
screen.render();

