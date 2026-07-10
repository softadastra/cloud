<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import {
    changePassword as changePasswordApi,
    updateProfile
  } from '$lib/api/auth';
  import { ApiError } from '$lib/api/types';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import { auth } from '$lib/stores/auth';

  let displayName = '';
  let savedDisplayName = '';

  let currentPassword = '';
  let newPassword = '';
  let confirmNewPassword = '';

  let showCurrentPassword = false;
  let showNewPassword = false;
  let showConfirmPassword = false;

  let profileMessage = '';
  let passwordMessage = '';
  let error = '';

  let savingProfile = false;
  let savingPassword = false;

  let copiedUserId = false;
  let copyResetTimer:
    | ReturnType<typeof setTimeout>
    | undefined;

  $: user = $auth.user;
  $: session = $auth.session;

  $: initial = (
    user?.display_name ||
    user?.name ||
    user?.email ||
    'U'
  )
    .slice(0, 1)
    .toUpperCase();

  $: normalizedDisplayName = displayName.trim();

  $: profileChanged =
    normalizedDisplayName !== savedDisplayName;

  $: passwordsMatch =
    !confirmNewPassword ||
    newPassword === confirmNewPassword;

  $: canChangePassword =
    Boolean(currentPassword) &&
    Boolean(newPassword) &&
    Boolean(confirmNewPassword) &&
    passwordsMatch;

  function formatDate(value?: string | number) {
    if (
      value === undefined ||
      value === null ||
      value === ''
    ) {
      return 'Unknown';
    }

    let date: Date;

    if (typeof value === 'number') {
      date = new Date(
        value < 1_000_000_000_000
          ? value * 1000
          : value
      );
    } else {
      const numericValue = Number(value);

      date =
        Number.isFinite(numericValue) &&
        value.trim() !== ''
          ? new Date(
              numericValue < 1_000_000_000_000
                ? numericValue * 1000
                : numericValue
            )
          : new Date(value);
    }

    if (Number.isNaN(date.getTime())) {
      return 'Unknown';
    }

    return new Intl.DateTimeFormat(undefined, {
      dateStyle: 'medium',
      timeStyle: 'short'
    }).format(date);
  }

  function clearMessages() {
    error = '';
    profileMessage = '';
    passwordMessage = '';
  }

  function handleProfileSubmit(event: SubmitEvent) {
    event.preventDefault();
    void saveProfile();
  }

  function handlePasswordSubmit(event: SubmitEvent) {
    event.preventDefault();
    void changePassword();
  }

  async function copyUserId() {
    if (!user?.id) {
      return;
    }

    try {
      await navigator.clipboard.writeText(user.id);

      copiedUserId = true;

      if (copyResetTimer) {
        clearTimeout(copyResetTimer);
      }

      copyResetTimer = setTimeout(() => {
        copiedUserId = false;
      }, 1800);
    } catch {
      error = 'Unable to copy the user ID.';
    }
  }

  async function saveProfile() {
    const currentSession = session;

    if (!currentSession) {
      return;
    }

    if (!normalizedDisplayName) {
      error = 'Display name is required.';
      return;
    }

    savingProfile = true;
    clearMessages();

    try {
      const updated = await updateProfile(
        currentSession.id,
        normalizedDisplayName
      );

      auth.setUser(updated.user);

      displayName =
        updated.user.display_name ||
        updated.user.name ||
        '';

      savedDisplayName = displayName.trim();

      profileMessage = 'Profile updated.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to update profile.';
    } finally {
      savingProfile = false;
    }
  }

  async function changePassword() {
    const currentSession = session;

    clearMessages();

    if (!currentSession) {
      return;
    }

    if (!currentPassword) {
      error = 'Current password is required.';
      return;
    }

    if (!newPassword) {
      error = 'New password is required.';
      return;
    }

    if (!confirmNewPassword) {
      error = 'Confirm the new password.';
      return;
    }

    if (newPassword !== confirmNewPassword) {
      error = 'New passwords do not match.';
      return;
    }

    savingPassword = true;

    try {
      await changePasswordApi({
        sessionId: currentSession.id,
        currentPassword,
        newPassword,
        confirmNewPassword
      });

      currentPassword = '';
      newPassword = '';
      confirmNewPassword = '';

      showCurrentPassword = false;
      showNewPassword = false;
      showConfirmPassword = false;

      passwordMessage =
        'Password changed successfully.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to change password.';
    } finally {
      savingPassword = false;
    }
  }

  function logout() {
    auth.clear();
    window.location.href = '/login';
  }

  onMount(() => {
    if (!session || !user) {
      void goto('/login');
      return;
    }

    displayName =
      user.display_name ||
      user.name ||
      '';

    savedDisplayName = displayName.trim();

    return () => {
      if (copyResetTimer) {
        clearTimeout(copyResetTimer);
      }
    };
  });
</script>

<svelte:head>
  <title>Account | Softadastra Cloud</title>
</svelte:head>

<PageHeader
  eyebrow="Account"
  title="Account settings"
/>

<p class="page-description">
  Manage your public profile, password and current account session.
</p>

<InlineError message={error} />

<div class="account-layout">
  <aside class="account-sidebar">
    <section
      class="account-card"
      aria-labelledby="account-identity-title"
    >
      <div class="account-identity">
        <span
          class="account-avatar"
          aria-hidden="true"
        >
          {initial}
        </span>

        <div>
          <h2 id="account-identity-title">
            {user?.display_name ||
              user?.name ||
              'Account'}
          </h2>

          <p>{user?.email ?? 'Unknown email'}</p>
        </div>
      </div>

      <div class="account-state">
        <span
          class:inactive={!session}
          class="session-indicator"
          aria-hidden="true"
        ></span>

        <span>
          {session
            ? 'Current session active'
            : 'No active session'}
        </span>
      </div>
    </section>

    <section
      class="session-card"
      aria-labelledby="session-title"
    >
      <div class="section-header">
        <div>
          <h2 id="session-title">
            Account details
          </h2>

          <p>
            Identity and current session information.
          </p>
        </div>
      </div>

      <dl class="account-properties">
        <div>
          <dt>Email</dt>

          <dd>{user?.email ?? 'Unknown'}</dd>
        </div>

        <div>
          <dt>Display name</dt>

          <dd>
            {user?.display_name ||
              user?.name ||
              'Not set'}
          </dd>
        </div>

        <div>
          <dt>Session</dt>

          <dd>
            <span
              class:inactive={!session}
              class="state-badge"
            >
              {session ? 'Active' : 'None'}
            </span>
          </dd>
        </div>

        <div>
          <dt>Session expires</dt>

          <dd>
            {formatDate(session?.expires_at)}
          </dd>
        </div>
      </dl>

      <div class="user-id-row">
        <div>
          <span>User ID</span>

          <code title={user?.id}>
            {user?.id ?? 'Unknown'}
          </code>
        </div>

        <button
          class:copied={copiedUserId}
          class="copy-id-button"
          type="button"
          disabled={!user?.id}
          aria-label={
            copiedUserId
              ? 'User ID copied'
              : 'Copy user ID'
          }
          title={
            copiedUserId
              ? 'Copied'
              : 'Copy user ID'
          }
          onclick={copyUserId}
        >
          {#if copiedUserId}
            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="m5 12 4 4L19 6"></path>
            </svg>
          {:else}
            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <rect
                x="8"
                y="8"
                width="11"
                height="11"
                rx="2"
              ></rect>

              <path
                d="M16 8V5a2 2 0 0 0-2-2H5a2 2 0 0 0-2 2v9a2 2 0 0 0 2 2h3"
              ></path>
            </svg>
          {/if}
        </button>
      </div>

      <div class="session-actions">
        <button
          class="sign-out-button"
          type="button"
          onclick={logout}
        >
          Sign out
        </button>
      </div>
    </section>
  </aside>

  <main class="account-content">
    <section
      class="settings-section"
      aria-labelledby="profile-title"
    >
      <div class="section-header">
        <div>
          <h2 id="profile-title">
            Profile
          </h2>

          <p>
            Information displayed across your
            Softadastra Cloud account.
          </p>
        </div>
      </div>

      <form
        class="settings-form"
        onsubmit={handleProfileSubmit}
      >
        <label>
          Email address

          <input
            value={user?.email ?? ''}
            type="email"
            readonly
            aria-describedby="email-help"
          />

          <small id="email-help">
            Email address changes are not available yet.
          </small>
        </label>

        <label>
          Display name

          <input
            bind:value={displayName}
            placeholder="Your name"
            required
            autocomplete="name"
          />

          <small>
            This name is used in workspace activity and
            account information.
          </small>
        </label>

        {#if profileMessage}
          <p
            class="section-success"
            role="status"
          >
            {profileMessage}
          </p>
        {/if}

        <div class="form-actions">
          <button
            type="submit"
            disabled={
              savingProfile ||
              !normalizedDisplayName ||
              !profileChanged
            }
          >
            {savingProfile
              ? 'Saving…'
              : 'Save profile'}
          </button>
        </div>
      </form>
    </section>

    <section
      class="settings-section"
      aria-labelledby="password-title"
    >
      <div class="section-header">
        <div>
          <h2 id="password-title">
            Password
          </h2>

          <p>
            Change the password used to access your account.
          </p>
        </div>
      </div>

      <form
        class="settings-form password-form"
        onsubmit={handlePasswordSubmit}
      >
        <label>
          Current password

          <span class="password-field">
            <input
              bind:value={currentPassword}
              type={
                showCurrentPassword
                  ? 'text'
                  : 'password'
              }
              required
              autocomplete="current-password"
            />

            <button
              class="password-toggle"
              type="button"
              aria-label={
                showCurrentPassword
                  ? 'Hide current password'
                  : 'Show current password'
              }
              onclick={() =>
                showCurrentPassword =
                  !showCurrentPassword}
            >
              {showCurrentPassword
                ? 'Hide'
                : 'Show'}
            </button>
          </span>
        </label>

        <div class="new-password-grid">
          <label>
            New password

            <span class="password-field">
              <input
                bind:value={newPassword}
                type={
                  showNewPassword
                    ? 'text'
                    : 'password'
                }
                required
                autocomplete="new-password"
              />

              <button
                class="password-toggle"
                type="button"
                aria-label={
                  showNewPassword
                    ? 'Hide new password'
                    : 'Show new password'
                }
                onclick={() =>
                  showNewPassword =
                    !showNewPassword}
              >
                {showNewPassword
                  ? 'Hide'
                  : 'Show'}
              </button>
            </span>
          </label>

          <label>
            Confirm new password

            <span class="password-field">
              <input
                class:invalid={
                  Boolean(confirmNewPassword) &&
                  !passwordsMatch
                }
                bind:value={confirmNewPassword}
                type={
                  showConfirmPassword
                    ? 'text'
                    : 'password'
                }
                required
                autocomplete="new-password"
                aria-invalid={
                  Boolean(confirmNewPassword) &&
                  !passwordsMatch
                }
              />

              <button
                class="password-toggle"
                type="button"
                aria-label={
                  showConfirmPassword
                    ? 'Hide password confirmation'
                    : 'Show password confirmation'
                }
                onclick={() =>
                  showConfirmPassword =
                    !showConfirmPassword}
              >
                {showConfirmPassword
                  ? 'Hide'
                  : 'Show'}
              </button>
            </span>

            {#if
              confirmNewPassword &&
              !passwordsMatch
            }
              <small class="password-error">
                Passwords do not match.
              </small>
            {/if}
          </label>
        </div>

        <div class="security-note">
          <svg
            viewBox="0 0 24 24"
            aria-hidden="true"
          >
            <path
              d="M12 3 4 7v5c0 5 3.4 8.7 8 10 4.6-1.3 8-5 8-10V7l-8-4Z"
            ></path>

            <path d="m9 12 2 2 4-4"></path>
          </svg>

          <p>
            Use a unique password that is not shared with
            another service. Passwords are never displayed
            or stored in plain text.
          </p>
        </div>

        {#if passwordMessage}
          <p
            class="section-success"
            role="status"
          >
            {passwordMessage}
          </p>
        {/if}

        <div class="form-actions">
          <button
            type="submit"
            disabled={
              savingPassword ||
              !canChangePassword
            }
          >
            {savingPassword
              ? 'Changing…'
              : 'Change password'}
          </button>
        </div>
      </form>
    </section>
  </main>
</div>

<style>
  .page-description {
    max-width: 720px;
    margin: -8px 0 18px;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.6;
  }

  .account-layout {
    display: grid;
    grid-template-columns:
      minmax(280px, 0.65fr)
      minmax(0, 1.35fr);
    gap: 16px;
    align-items: start;
  }

  .account-sidebar,
  .account-content {
    display: grid;
    min-width: 0;
    gap: 16px;
  }

  .account-sidebar {
    position: sticky;
    top: 24px;
  }

  .account-card,
  .session-card,
  .settings-section {
    min-width: 0;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  /* Account identity */

  .account-card {
    display: grid;
    gap: 16px;
    padding: 18px;
  }

  .account-identity {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 13px;
  }

  .account-avatar {
    display: grid;
    width: 46px;
    height: 46px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 50%;
    background: var(--brand-faint);
    color: var(--brand-bright);
    font-size: 16px;
    font-weight: 700;
  }

  .account-identity > div {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .account-identity h2 {
    overflow: hidden;
    color: var(--text);
    font-size: 15px;
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .account-identity p {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 11.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .account-state {
    display: flex;
    align-items: center;
    gap: 8px;
    border-top: 1px solid var(--line-soft);
    padding-top: 13px;
    color: var(--text-muted);
    font-size: 11px;
  }

  .session-indicator {
    width: 7px;
    height: 7px;
    flex: 0 0 auto;
    border-radius: 50%;
    background: var(--success);
  }

  .session-indicator.inactive {
    background: var(--text-faint);
  }

  /* Shared section header */

  .section-header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 14px;
    padding: 14px 16px;
    border-bottom: 1px solid var(--line-soft);
  }

  .section-header > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .section-header h2 {
    color: var(--text);
    font-size: 13.5px;
  }

  .section-header p {
    color: var(--text-muted);
    font-size: 11.5px;
    line-height: 1.5;
  }

  /* Account properties */

  .account-properties {
    display: grid;
    margin: 0;
  }

  .account-properties > div {
    display: grid;
    grid-template-columns: 105px minmax(0, 1fr);
    gap: 12px;
    align-items: center;
    min-height: 45px;
    padding: 9px 14px;
    border-bottom: 1px solid var(--line-soft);
  }

  .account-properties dt {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .account-properties dd {
    min-width: 0;
    margin: 0;
    color: var(--text-soft);
    font-size: 11px;
    font-weight: 550;
    overflow-wrap: anywhere;
  }

  .state-badge {
    display: inline-flex;
    width: fit-content;
    min-height: 20px;
    align-items: center;
    border: 1px solid rgba(74, 222, 128, 0.26);
    border-radius: 999px;
    background: rgba(74, 222, 128, 0.08);
    color: var(--success);
    padding: 0 7px;
    font-size: 10px;
  }

  .state-badge.inactive {
    border-color: var(--line-strong);
    background: var(--bg-elevated);
    color: var(--text-muted);
  }

  .user-id-row {
    display: grid;
    grid-template-columns: minmax(0, 1fr) 30px;
    gap: 10px;
    align-items: center;
    padding: 11px 14px;
    border-bottom: 1px solid var(--line-soft);
  }

  .user-id-row > div {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .user-id-row span {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .user-id-row code {
    min-width: 0;
    overflow: hidden;
    color: var(--text-soft);
    font-size: 9.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .copy-id-button {
    display: grid;
    width: 28px;
    height: 28px;
    min-height: 28px;
    place-items: center;
    border: 1px solid var(--line-strong);
    background: transparent;
    color: var(--text-muted);
    padding: 0;
  }

  .copy-id-button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
  }

  .copy-id-button.copied {
    border-color: var(--brand-line);
    color: var(--brand);
  }

  .copy-id-button svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .session-actions {
    display: flex;
    justify-content: flex-end;
    padding: 12px 14px;
  }

  .sign-out-button {
    min-height: 30px;
    border-color: var(--danger-line);
    background: transparent;
    color: var(--danger);
    padding: 0 11px;
    font-size: 11px;
  }

  .sign-out-button:hover:not(:disabled) {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  /* Forms */

  .settings-form {
    display: grid;
    gap: 15px;
    padding: 16px;
  }

  .settings-form label {
    position: relative;
  }

  .settings-form label > small {
    color: var(--text-muted);
    font-size: 10.5px;
    font-weight: 400;
    line-height: 1.45;
  }

  .settings-form input[readonly] {
    background: var(--bg-ink-soft);
    color: var(--text-muted);
    cursor: not-allowed;
  }

  .new-password-grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 14px;
  }

  .password-field {
    position: relative;
    display: block;
  }

  .password-field input {
    padding-right: 58px;
  }

  .password-field input.invalid {
    border-color: var(--danger);
  }

  .password-toggle {
    position: absolute;
    top: 50%;
    right: 6px;
    min-height: 27px;
    border: 0;
    background: transparent;
    color: var(--link);
    padding: 0 7px;
    font-size: 10.5px;
    font-weight: 550;
    transform: translateY(-50%);
  }

  .password-toggle:hover:not(:disabled) {
    background: var(--info-faint);
    color: var(--link-hover);
    transform: translateY(-50%);
  }

  .password-error {
    color: var(--danger) !important;
  }

  .security-note {
    display: grid;
    grid-template-columns: 18px minmax(0, 1fr);
    gap: 9px;
    align-items: start;
    border: 1px solid var(--line-soft);
    border-radius: var(--radius-sm);
    background: var(--bg-ink-soft);
    padding: 11px;
  }

  .security-note svg {
    width: 17px;
    height: 17px;
    margin-top: 1px;
    fill: none;
    stroke: var(--text-muted);
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .security-note p {
    color: var(--text-muted);
    font-size: 10.5px;
    line-height: 1.55;
  }

  .section-success {
    border: 1px solid rgba(74, 222, 128, 0.22);
    border-radius: var(--radius-sm);
    background: rgba(74, 222, 128, 0.07);
    color: var(--success);
    padding: 9px 11px;
    font-size: 11px;
  }

  .form-actions {
    display: flex;
    justify-content: flex-end;
    border-top: 1px solid var(--line-soft);
    padding-top: 14px;
  }

  @media (max-width: 860px) {
    .account-layout {
      grid-template-columns: 1fr;
    }

    .account-sidebar {
      position: static;
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }
  }

  @media (max-width: 680px) {
    .account-sidebar {
      grid-template-columns: 1fr;
    }

    .new-password-grid {
      grid-template-columns: 1fr;
    }
  }

  @media (max-width: 480px) {
    .account-properties > div {
      grid-template-columns: 1fr;
      gap: 4px;
    }

    .form-actions button {
      width: 100%;
    }

    .settings-form {
      padding: 14px;
    }

    .session-actions {
      align-items: stretch;
      flex-direction: column;
    }

    .sign-out-button {
      width: 100%;
    }
  }
</style>
