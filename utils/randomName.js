import fs from "fs";

export function generateUniqueNames(count) {
    const words = fs.readFileSync("wordlist.txt", "utf-8").split("\n");
    const selected = new Set();
    const results = [];

    while (results.length < count) {
        const name = words[Math.floor(Math.random() * words.length)].trim();
        const symbol = name.slice(0, 3).toUpperCase();
        const key = `${name}-${symbol}`;
        if (!selected.has(key) && name.length > 2) {
            selected.add(key);
            results.push({ name, symbol });
        }
    }

    return results;
}
