using DSharpPlus;
using DSharpPlus.CommandsNext;
using DSharpPlus.Interactivity;
using DSharpPlus.Interactivity.Enums;
using DSharpPlus.Interactivity.Extensions;
using DSharpPlus.Lavalink;
using DSharpPlus.Net;
using DSharpPlus.SlashCommands;
using System.Diagnostics;
using YotsubaSharp.API;
using YotsubaSharp.Commands;
using YotsubaSharp.Database;
using YotsubaSharp.Handlers;

namespace YotsubaSharp
{
    public class Program
    {
        public readonly static DiscordClient Client = new(new DiscordConfiguration()
        {
            Intents = DiscordIntents.All,
            Token = "token", // Add quotes around the token
#if DEBUG
            MinimumLogLevel = Microsoft.Extensions.Logging.LogLevel.Debug,
#else
    MinimumLogLevel = Microsoft.Extensions.Logging.LogLevel.Information,
#endif
        });


        static void Main()
        {
            List<Process> ExistingProcs = new();
            ExistingProcs.AddRange(Process.GetProcessesByName("java"));
            ExistingProcs.AddRange(Process.GetProcessesByName("VsDebugConsole"));
            ExistingProcs.AddRange(Process.GetProcessesByName("YotsubaSharp"));
            foreach (Process process in ExistingProcs)
            {
                try
                {
                    process.Kill(true);
                }
                catch (Exception) { }
            }
            MainAsync().GetAwaiter().GetResult();
        }

        public static readonly TimerManager<AutoDeleteData> AutoDeleteMgr = new("AutoDelete", AutoDeleteHandler.AutoDeleteCallback);
        public static readonly TimerManager<PunishmentDocument> PunishmentMgr = new("PunishmentTimer", PunishmentManager.OnPunishmentTimerEnded);
        public static readonly TimerManager<BankData> BankMgr = new("BankTimer", EconomyManager.ChargeBankInterest);
        //public static readonly TimerManager<PersistedRoleDocument> RolePersistMgr = new("PersistTimer", RolePersistManager.OnPersistTimerEnd);

        public delegate Task BotReadyDelegate();
        public static event BotReadyDelegate? BotReady;

        static async Task MainAsync()
        {
            var Commands = Client.UseCommandsNext(new CommandsNextConfiguration()
            {
                StringPrefixes = new[] {"."}
            });

            var SlashCommands = Client.UseSlashCommands();

            // Set up interactivity
            Client.UseInteractivity(new InteractivityConfiguration()
            {
                PollBehaviour = PollBehaviour.KeepEmojis,
                Timeout = TimeSpan.FromSeconds(30),
                AckPaginationButtons = true
            });

            // Register command modules
            Commands.RegisterCommands<DebugModule>();
            SlashCommands.RegisterCommands<DebugModuleSlash>();

            Commands.RegisterCommands<AIModule>();
            SlashCommands.RegisterCommands<AIModuleSlash>();

            Commands.RegisterCommands<ShitpostModule>();
            SlashCommands.RegisterCommands<ShitpostModuleSlash>();

            Commands.RegisterCommands<FunModule>();
            SlashCommands.RegisterCommands<FunModuleSlash>();

            Commands.RegisterCommands<AdminModule>();
            Commands.RegisterCommands<EconomyModule>();



            Database.Client.InitDB(Client);

            Handlers.OnlyVideosHandler.Handle(
                Client,
                await Config.GetConfigValue<ulong>(new ConfigID() { guildID = Constants.ShitpostID }, ConfigProperties.VideoChannelID)
                );

            Handlers.PinboardHandler.HandleReactPin(
                Client,
                await Config.GetConfigValue<ulong>(new ConfigID() { guildID = Constants.ShitpostID }, ConfigProperties.PinChannelID),
                5,
                new string[] { ":upvote:", ":extendedkek:", ":hesrightyouknow:" }
                );
            Handlers.PinboardHandler.HandlePin(
                Client,
                await Config.GetConfigValue<ulong>(new ConfigID() { guildID = Constants.ShitpostID }, ConfigProperties.PinChannelID));
            Handlers.JoinLeaveHandler.Handle(
                Client,
                await Config.GetConfigValue<ulong>(new ConfigID() { guildID = Constants.ShitpostID }, ConfigProperties.LogChannelID),
                await Config.GetConfigValue<ulong>(new ConfigID() { guildID = Constants.ShitpostID }, ConfigProperties.WelcomeChannelID),
                await Config.GetConfigValue<ulong>(new ConfigID() { guildID = Constants.ShitpostID }, ConfigProperties.JoinRole)
                );
            await Handlers.LogHandler.Handle(
                Client,
                await Config.GetConfigValue<ulong>(new ConfigID() { guildID = Constants.ShitpostID }, ConfigProperties.LogChannelID)
                );
            await Handlers.AutoModHandler.HandleAsync(Client,
                await Config.GetConfigValue<ulong>(new ConfigID() { guildID = Constants.ShitpostID }, ConfigProperties.LogChannelID)
                );
            Handlers.AutoResponseHandler.Handle(Client);
            Handlers.InviteHandler.Handle(Client
            //, await Config.GetConfigValue<ulong>(new ConfigID() { guildID = Constants.ShitpostID }, ConfigProperties.LogChannelID)
            );
            Handlers.CommandAttributeHandler.HandleCommandError(Commands);
            EconomyManager.Handle(Client);
            await Database.Markov.InitChain();
            Replika.InitAI(Client);

            //LAVALINK
            Commands.RegisterCommands<MusicModule>();
            var endpoint = new ConnectionEndpoint
            {
                Hostname = "127.0.0.1", // From your server configuration.
                Port = 2333 // From your server configuration
            };

            var lavalinkConfig = new LavalinkConfiguration
            {
                Password = "sexniggers", // From your server configuration.
                RestEndpoint = endpoint,
                SocketEndpoint = endpoint
            };

            Process.Start(new ProcessStartInfo()
            {
                Arguments = @"/Ccolor 0a && java -jar Lavalink.jar",
                FileName = "cmd",
                WorkingDirectory = @"..\..\..\Lavalink"

            });

            await Task.Delay(8000);

            var lavalink = Client.UseLavalink();
            Client.GuildAvailable += Client_GuildAvailable;
            Client.VoiceStateUpdated += VoiceManager.OnVoiceStateUpdated;
            await Client.ConnectAsync();
            await lavalink.ConnectAsync(lavalinkConfig);

            EconomyManager.Handle(Client);

            if (BotReady != null)
                await BotReady.Invoke();

            await Task.Delay(-1);
        }

        private static Task Client_GuildAvailable(DiscordClient sender, DSharpPlus.EventArgs.GuildCreateEventArgs e)
        {
            _ = Task.Run(async () =>
            {
                //if (e.Guild.Id == Constants.ShitpostID)
                //ActivityHandler.Handle(e.Guild);
                await InviteManager.CacheInviteOnStartAsync(e.Guild);
            });
            return Task.CompletedTask;
        }
    }
}