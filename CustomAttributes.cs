using DSharpPlus.Entities;
using DSharpPlus.SlashCommands;
using YotsubaSharp.Database;

namespace DSharpPlus.CommandsNext.Attributes
{
    public sealed class ShitpostAttribute : CheckBaseAttribute
    {
        public ShitpostAttribute()
        {
        }

        public override Task<bool> ExecuteCheckAsync(CommandContext ctx, bool help)
        {
#if DEBUG
            if(ctx.Guild.Id == 964200228481798254) // dev server ID
                return Task.FromResult(true);
#endif
            return Task.FromResult(ctx.Guild != null && ctx.Guild.Id == YotsubaSharp.Constants.ShitpostID);
        }
    }

    public sealed class SlashShitpostAttribute : SlashCheckBaseAttribute
    {
        public SlashShitpostAttribute()
        {
        }

        override public Task<bool> ExecuteChecksAsync(InteractionContext ctx)
        {
#if DEBUG
            if(ctx.Guild.Id == 964200228481798254) // dev server ID
                return Task.FromResult(true);
#endif
            return Task.FromResult(ctx.Guild != null && ctx.Guild.Id == YotsubaSharp.Constants.ShitpostID);
        }
    }

    /// <summary>
    /// Checks that the user running a command has a matching id or, alternatively, has admin perms.
    /// </summary>
    public sealed class UserIDAttribute : CheckBaseAttribute
    {
        readonly ulong[] userIDs;
        readonly bool allowAdmins;
        /// <summary>
        /// Checks that the user running a command has a matching id or, alternatively, has admin perms.
        /// </summary>
        /// <param name="allowAdmins">Whether users with admin perms should satisfy this check or not.</param>
        /// <param name="userIDs">UserIDs to allow.</param>
        public UserIDAttribute(bool allowAdmins, params ulong[] userIDs)
        {
            this.allowAdmins = allowAdmins;
            this.userIDs = userIDs;
        }

        public override Task<bool> ExecuteCheckAsync(CommandContext ctx, bool help)
        {
            if (ctx.Guild == null)
                return Task.FromResult(false);
            return Task.FromResult(userIDs.Contains(ctx.User.Id) || (allowAdmins && ((DiscordMember)ctx.User).Permissions.HasPermission(Permissions.Administrator)));
        }
    }

    /// <summary>
    /// Checks that the user running a command has a matching id or, alternatively, has admin perms.
    /// </summary>
    public sealed class SlashUserIDAttribute : SlashCheckBaseAttribute
    {
        readonly ulong[] userIDs;
        readonly bool allowAdmins;
        /// <summary>
        /// Checks that the user running a command has a matching id or, alternatively, has admin perms.
        /// </summary>
        /// <param name="allowAdmins">Whether users with admin perms should satisfy this check or not.</param>
        /// <param name="userIDs">UserIDs to allow.</param>
        public SlashUserIDAttribute(bool allowAdmins, params ulong[] userIDs)
        {
            this.allowAdmins = allowAdmins;
            this.userIDs = userIDs;
        }

        public override Task<bool> ExecuteChecksAsync(InteractionContext ctx)
        {
            if (ctx.Guild == null)
                return Task.FromResult(false);
            return Task.FromResult(userIDs.Contains(ctx.User.Id) || (allowAdmins && ((DiscordMember)ctx.User).Permissions.HasPermission(Permissions.Administrator)));
        }
    }

    public sealed class HasConfigRoleAttribute : CheckBaseAttribute
    {
        string ConfigProperty;
        public HasConfigRoleAttribute(string property)
        {
            this.ConfigProperty = property;
        }

        public override async Task<bool> ExecuteCheckAsync(CommandContext ctx, bool help)
        {
            if (ctx.Member == null)
                return false;
            if ((ctx.Member.Permissions & Permissions.ManageRoles) != 0)
                return true;
            return ctx.Member.Roles.Select(x => x.Id).Contains(await Config.GetConfigValue<ulong>(new ConfigID() { guildID = ctx.Guild.Id }, ConfigProperty, 0));
        }
    }

    public sealed class SlashHasConfigRoleAttribute : SlashCheckBaseAttribute
    {
        string ConfigProperty;
        public SlashHasConfigRoleAttribute(string property)
        {
            this.ConfigProperty = property;
        }

        public override async Task<bool> ExecuteChecksAsync(InteractionContext ctx)
        {
            if (ctx.Member == null)
                return false;
            if ((ctx.Member.Permissions & Permissions.ManageRoles) != 0)
                return true;
            return ctx.Member.Roles.Select(x => x.Id).Contains(await Config.GetConfigValue<ulong>(new ConfigID() { guildID = ctx.Guild.Id }, ConfigProperty, 0));
        }
    }

    public sealed class HasItemAttribute : CheckBaseAttribute
    {
        Item item;
        uint quantity;
        public HasItemAttribute(Item item, uint quantity = 1)
        {
            this.item = item;
            this.quantity = quantity;
        }

        public override async Task<bool> ExecuteCheckAsync(CommandContext ctx, bool help)
        {
            if (ctx.Guild == null)
                return false;
            return await EconomyManager.UserHasItem(ctx.User, item, quantity);
        }
    }

    public sealed class SlashHasItemAttribute : SlashCheckBaseAttribute
    {
        Item item;
        uint quantity;
        public SlashHasItemAttribute(Item item, uint quantity = 1)
        {
            this.item = item;
            this.quantity = quantity;
        }

        public override async Task<bool> ExecuteChecksAsync(InteractionContext ctx)
        {
            if (ctx.Guild == null)
                return false;
            return await EconomyManager.UserHasItem(ctx.User, item, quantity);
        }
    }

    public sealed class HasBalanceAttribute : CheckBaseAttribute
    {
        uint balance;
        public HasBalanceAttribute(uint balance)
        {
            this.balance = balance;
        }

        public override async Task<bool> ExecuteCheckAsync(CommandContext ctx, bool help)
        {
            if (help)
                return true;
            if (ctx.Guild == null)
                return false;
            bool sufficientFunds = await EconomyManager.GetBalance(ctx.User) >= balance;
            if (!sufficientFunds)
                await ctx.RespondAsync($"You need at least ${balance:n0} to use this command!");
            return sufficientFunds;
        }
    }

    public sealed class SlashHasBalanceAttribute : SlashCheckBaseAttribute
    {
        uint balance;
        public SlashHasBalanceAttribute(uint balance)
        {
            this.balance = balance;
        }

        public override async Task<bool> ExecuteChecksAsync(InteractionContext ctx)
        {
            if (ctx.Guild == null)
                return false;
            bool sufficientFunds = await EconomyManager.GetBalance(ctx.User) >= balance;
            if (!sufficientFunds)
                await ctx.CreateResponseAsync($"You need at least ${balance:n0} to use this command!");
            return sufficientFunds;
        }
    }
}
