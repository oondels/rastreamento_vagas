/** @type {import('tailwindcss').Config} */
export default {
  content: ["./index.html", "./src/**/*.{vue,js}"],
  theme: {
    extend: {
      colors: {
        occupied: {
          base: "#dc2626",
          soft: "#450a0a"
        },
        available: {
          base: "#16a34a",
          soft: "#052e16"
        }
      },
      boxShadow: {
        panel: "0 24px 64px rgba(15, 23, 42, 0.35)"
      }
    }
  },
  plugins: []
};
