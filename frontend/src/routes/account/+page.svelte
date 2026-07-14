<script lang="ts">
  import { browser } from '$app/environment';
  import { goto } from '$app/navigation';
  import { onMount, tick } from 'svelte';

  import {
    changePassword as changePasswordApi,
    deleteAvatar,
    updateProfile,
    uploadAvatar
  } from '$lib/api/auth';

  import {
    getMySupporterStatus
  } from '$lib/api/support';

  import {
    ApiError,
    type MySupporterStatus,
    type User
  } from '$lib/api/types';

  import InlineError from '$lib/components/InlineError.svelte';
  import SupporterBadge from '$lib/components/SupporterBadge.svelte';

  import { auth } from '$lib/stores/auth';

  type AccountSection =
    | 'profile'
    | 'public'
    | 'security'
    | 'session';

  const ACCOUNT_SECTIONS: AccountSection[] = [
    'profile',
    'public',
    'security',
    'session'
  ];

  const API_BASE_URL =
    import.meta.env.VITE_API_BASE_URL ??
    (
      browser
        ? `${window.location.protocol}//${window.location.hostname}:8080`
        : ''
    );

  let activeSection: AccountSection = 'profile';

  let displayName = '';
  let username = '';
  let bio = '';
  let websiteUrl = '';
  let githubUrl = '';
  let publicProfileEnabled = false;

  let savedDisplayName = '';
  let savedUsername = '';
  let savedBio = '';
  let savedWebsiteUrl = '';
  let savedGithubUrl = '';
  let savedPublicProfileEnabled = false;

  let avatarInput: HTMLInputElement;

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
  let uploadingAvatar = false;
  let deletingAvatar = false;

  let supporterStatus:
    MySupporterStatus['supporter'] = null;

  let loadingSupporterStatus = false;

  let copiedUserId = false;

  let copyResetTimer:
    | ReturnType<typeof setTimeout>
    | undefined;

  let showAvatarRemoval = false;
  let avatarRemovalDialog:
    HTMLDialogElement | null = null;

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

  $: normalizedDisplayName =
    displayName.trim();

  $: normalizedUsername =
    username.trim().toLowerCase();

  $: normalizedBio =
    bio.trim();

  $: normalizedWebsiteUrl =
    websiteUrl.trim();

  $: normalizedGithubUrl =
    githubUrl.trim();

  $: avatarUrl =
    user?.avatar_url
      ? user.avatar_url.startsWith('http')
        ? user.avatar_url
        : `${API_BASE_URL}${user.avatar_url}`
      : '';

  $: savedPublicProfileUrl =
    savedUsername
      ? `/u/${savedUsername}`
      : '';

  $: canViewPublicProfile =
    savedPublicProfileEnabled &&
    Boolean(savedUsername);

  $: profileChanged =
    normalizedDisplayName !== savedDisplayName ||
    normalizedUsername !== savedUsername ||
    normalizedBio !== savedBio ||
    normalizedWebsiteUrl !== savedWebsiteUrl ||
    normalizedGithubUrl !== savedGithubUrl ||
    publicProfileEnabled !==
      savedPublicProfileEnabled;

  $: passwordsMatch =
    !confirmNewPassword ||
    newPassword === confirmNewPassword;

  $: canChangePassword =
    Boolean(currentPassword) &&
    Boolean(newPassword) &&
    Boolean(confirmNewPassword) &&
    passwordsMatch;

  $: passwordStrength =
    calculatePasswordStrength(newPassword);

  $: passwordStrengthLabel =
    strengthLabel(passwordStrength);

  $: profileCompletion =
    calculateProfileCompletion();

  $: accountName =
    normalizedDisplayName ||
    user?.display_name ||
    user?.name ||
    'Your account';

  function calculateProfileCompletion() {
    const values = [
      Boolean(normalizedDisplayName),
      Boolean(normalizedUsername),
      Boolean(normalizedBio),
      Boolean(
        normalizedWebsiteUrl ||
        normalizedGithubUrl
      ),
      Boolean(user?.avatar_url)
    ];

    const completed =
      values.filter(Boolean).length;

    return Math.round(
      (completed / values.length) * 100
    );
  }

  function calculatePasswordStrength(
    value: string
  ) {
    if (!value) {
      return 0;
    }

    let score = 0;

    if (value.length >= 8) {
      score += 1;
    }

    if (value.length >= 12) {
      score += 1;
    }

    if (
      /[a-z]/.test(value) &&
      /[A-Z]/.test(value)
    ) {
      score += 1;
    }

    if (
      /\d/.test(value) &&
      /[^A-Za-z0-9]/.test(value)
    ) {
      score += 1;
    }

    return Math.min(score, 4);
  }

  function strengthLabel(
    value: number
  ) {
    if (value === 0) {
      return 'Not evaluated';
    }

    if (value === 1) {
      return 'Weak';
    }

    if (value === 2) {
      return 'Fair';
    }

    if (value === 3) {
      return 'Good';
    }

    return 'Strong';
  }

  function formatDate(
    value?: string | number
  ) {
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
        value <
        1_000_000_000_000
          ? value * 1000
          : value
      );
    } else {
      const numericValue =
        Number(value);

      date =
        Number.isFinite(numericValue) &&
        value.trim() !== ''
          ? new Date(
              numericValue <
              1_000_000_000_000
                ? numericValue * 1000
                : numericValue
            )
          : new Date(value);
    }

    if (
      Number.isNaN(date.getTime())
    ) {
      return 'Unknown';
    }

    return new Intl.DateTimeFormat(
      undefined,
      {
        dateStyle: 'medium',
        timeStyle: 'short'
      }
    ).format(date);
  }

  function clearMessages() {
    error = '';
    profileMessage = '';
    passwordMessage = '';
  }

  function syncProfileForm(
    nextUser: User | null
  ) {
    displayName =
      nextUser?.display_name ||
      nextUser?.name ||
      '';

    username =
      nextUser?.username || '';

    bio =
      nextUser?.bio || '';

    websiteUrl =
      nextUser?.website_url || '';

    githubUrl =
      nextUser?.github_url || '';

    publicProfileEnabled =
      Boolean(
        nextUser?.public_profile_enabled
      );

    savedDisplayName =
      displayName.trim();

    savedUsername =
      username.trim().toLowerCase();

    savedBio =
      bio.trim();

    savedWebsiteUrl =
      websiteUrl.trim();

    savedGithubUrl =
      githubUrl.trim();

    savedPublicProfileEnabled =
      publicProfileEnabled;
  }

  function setSection(
    section: AccountSection
  ) {
    activeSection = section;
    error = '';

    if (!browser) {
      return;
    }

    const url =
      new URL(window.location.href);

    url.searchParams.set(
      'section',
      section
    );

    history.replaceState(
      null,
      '',
      `${url.pathname}${url.search}`
    );
  }

  function handleProfileSubmit(
    event: SubmitEvent
  ) {
    event.preventDefault();
    void saveProfile();
  }

  function handlePasswordSubmit(
    event: SubmitEvent
  ) {
    event.preventDefault();
    void changePassword();
  }

  async function copyUserId() {
    if (!user?.id) {
      return;
    }

    try {
      await navigator.clipboard.writeText(
        user.id
      );

      copiedUserId = true;

      if (copyResetTimer) {
        clearTimeout(
          copyResetTimer
        );
      }

      copyResetTimer = setTimeout(
        () => {
          copiedUserId = false;
        },
        1800
      );
    } catch {
      error =
        'Unable to copy the user ID.';
    }
  }

  async function saveProfile() {
    const currentSession = session;

    if (!currentSession) {
      return;
    }

    if (!normalizedDisplayName) {
      error =
        'Display name is required.';

      return;
    }

    if (
      normalizedUsername &&
      !/^[a-z0-9_-]+$/.test(
        normalizedUsername
      )
    ) {
      error =
        'Username can only contain letters, numbers, hyphens and underscores.';

      return;
    }

    if (
      publicProfileEnabled &&
      !normalizedUsername
    ) {
      error =
        'Choose a username before enabling the public profile.';

      return;
    }

    savingProfile = true;
    clearMessages();

    try {
      const updated =
        await updateProfile(
          currentSession.id,
          {
            display_name:
              normalizedDisplayName,

            username:
              normalizedUsername,

            bio:
              normalizedBio,

            website_url:
              normalizedWebsiteUrl,

            github_url:
              normalizedGithubUrl,

            public_profile_enabled:
              publicProfileEnabled
          }
        );

      auth.setUser(
        updated.user
      );

      syncProfileForm(
        updated.user
      );

      profileMessage =
        activeSection === 'public'
          ? 'Public profile settings updated.'
          : 'Profile updated.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to update profile.';
    } finally {
      savingProfile = false;
    }
  }

  function openAvatarPicker() {
    avatarInput?.click();
  }

  async function handleAvatarChange(
    event: Event
  ) {
    const input =
      event.currentTarget as HTMLInputElement;

    const file =
      input.files?.[0];

    if (!file) {
      return;
    }

    clearMessages();

    if (
      ![
        'image/jpeg',
        'image/png',
        'image/webp'
      ].includes(file.type)
    ) {
      error =
        'Avatar must be a JPG, PNG or WebP image.';

      input.value = '';
      return;
    }

    if (
      file.size >
      2 * 1024 * 1024
    ) {
      error =
        'Choose an image smaller than 2MB.';

      input.value = '';
      return;
    }

    uploadingAvatar = true;

    try {
      const updated =
        await uploadAvatar(file);

      auth.setUser(
        updated.user
      );

      profileMessage =
        'Profile photo updated.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to upload avatar.';
    } finally {
      uploadingAvatar = false;
      input.value = '';
    }
  }

  async function openAvatarRemoval() {
    if (
      !user?.avatar_url ||
      deletingAvatar
    ) {
      return;
    }

    showAvatarRemoval = true;

    await tick();

    avatarRemovalDialog?.focus();
  }

  function closeAvatarRemoval() {
    if (deletingAvatar) {
      return;
    }

    showAvatarRemoval = false;
  }

  async function confirmAvatarRemoval() {
    if (!user?.avatar_url) {
      return;
    }

    clearMessages();
    deletingAvatar = true;

    try {
      await deleteAvatar();

      auth.setUser({
        ...user,
        avatar_url: ''
      });

      profileMessage =
        'Profile photo removed.';

      showAvatarRemoval = false;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to remove avatar.';
    } finally {
      deletingAvatar = false;
    }
  }

  async function changePassword() {
    const currentSession = session;

    clearMessages();

    if (!currentSession) {
      return;
    }

    if (!currentPassword) {
      error =
        'Current password is required.';

      return;
    }

    if (!newPassword) {
      error =
        'New password is required.';

      return;
    }

    if (!confirmNewPassword) {
      error =
        'Confirm the new password.';

      return;
    }

    if (
      newPassword !==
      confirmNewPassword
    ) {
      error =
        'New passwords do not match.';

      return;
    }

    savingPassword = true;

    try {
      await changePasswordApi({
        sessionId:
          currentSession.id,

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

  async function loadSupporterStatus() {
    if (!session) {
      return;
    }

    loadingSupporterStatus = true;

    try {
      const data =
        await getMySupporterStatus();

      supporterStatus =
        data.supporter;
    } catch {
      supporterStatus = null;
    } finally {
      loadingSupporterStatus = false;
    }
  }

  function logout() {
    auth.clear();
    window.location.href = '/login';
  }

  function handleWindowKeydown(
    event: KeyboardEvent
  ) {
    if (
      event.key === 'Escape' &&
      showAvatarRemoval &&
      !deletingAvatar
    ) {
      event.preventDefault();
      closeAvatarRemoval();
    }
  }

  onMount(() => {
    if (
      !session ||
      !user
    ) {
      void goto('/login');
      return;
    }

    const requestedSection =
      new URLSearchParams(
        window.location.search
      ).get('section');

    if (
      requestedSection &&
      ACCOUNT_SECTIONS.includes(
        requestedSection as AccountSection
      )
    ) {
      activeSection =
        requestedSection as AccountSection;
    }

    syncProfileForm(user);
    void loadSupporterStatus();

    return () => {
      if (copyResetTimer) {
        clearTimeout(
          copyResetTimer
        );
      }
    };
  });
</script>

<svelte:head>
  <title>
    Account settings | Softadastra Cloud
  </title>

  <meta
    name="description"
    content="Manage your Softadastra Cloud identity, public profile, password and current account session."
  />
</svelte:head>

<svelte:window
  onkeydown={handleWindowKeydown}
/>

<div class="account-page">
  <!-- ===================================================
       PAGE HEADER
       =================================================== -->
  <header class="page-header">
    <div class="page-heading">
      <p class="page-eyebrow">
        <span aria-hidden="true"></span>
        Personal settings
      </p>

      <h1>Account</h1>

      <p>
        Manage your identity, public presence,
        security and current session.
      </p>
    </div>

    <div class="header-account">
      <span
        class="header-account__avatar"
        aria-hidden="true"
      >
        {#if avatarUrl}
          <img
            src={avatarUrl}
            alt=""
          />
        {:else}
          {initial}
        {/if}
      </span>

      <span>
        <strong>{accountName}</strong>
        <small>
          {user?.email ?? 'Unknown email'}
        </small>
      </span>
    </div>
  </header>

  <InlineError message={error} />

  {#if profileMessage}
    <div
      class="success-message"
      role="status"
    >
      <span aria-hidden="true">
        <svg
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2.2"
          stroke-linecap="round"
          stroke-linejoin="round"
        >
          <path d="m5 12 4 4L19 6"></path>
        </svg>
      </span>

      <p>{profileMessage}</p>
    </div>
  {/if}

  {#if passwordMessage}
    <div
      class="success-message"
      role="status"
    >
      <span aria-hidden="true">
        <svg
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2.2"
          stroke-linecap="round"
          stroke-linejoin="round"
        >
          <path d="m5 12 4 4L19 6"></path>
        </svg>
      </span>

      <p>{passwordMessage}</p>
    </div>
  {/if}

  <div class="account-shell">
    <!-- =================================================
         ACCOUNT NAVIGATION
         ================================================= -->
    <aside class="account-navigation">
      <section class="identity-card">
        <div class="identity-card__main">
          <span
            class="identity-avatar"
            aria-hidden="true"
          >
            {#if avatarUrl}
              <img
                src={avatarUrl}
                alt=""
              />
            {:else}
              {initial}
            {/if}
          </span>

          <div>
            <strong>{accountName}</strong>

            <span>
              {normalizedUsername
                ? `@${normalizedUsername}`
                : user?.email ??
                  'Softadastra account'}
            </span>
          </div>
        </div>

        {#if supporterStatus}
          <div class="identity-supporter">
            <SupporterBadge
              tier={supporterStatus.tier}
            />
          </div>
        {/if}

        <div class="completion">
          <div class="completion__heading">
            <span>Profile completion</span>
            <strong>
              {profileCompletion}%
            </strong>
          </div>

          <span
            class="completion__track"
            aria-hidden="true"
          >
            <span
              style={`width: ${profileCompletion}%`}
            ></span>
          </span>
        </div>
      </section>

      <nav
        class="settings-navigation"
        aria-label="Account settings"
      >
        <button
          class:active={
            activeSection === 'profile'
          }
          type="button"
          aria-pressed={
            activeSection === 'profile'
          }
          onclick={() =>
            setSection('profile')}
        >
          <span
            class="settings-navigation__icon"
            aria-hidden="true"
          >
            <svg
              viewBox="0 0 24 24"
            >
              <circle
                cx="12"
                cy="8"
                r="4"
              ></circle>

              <path
                d="M4 21a8 8 0 0 1 16 0"
              ></path>
            </svg>
          </span>

          <span>
            <strong>Profile</strong>
            <small>
              Identity and contact links
            </small>
          </span>

          {#if profileChanged}
            <i
              class="unsaved-dot"
              aria-label="Unsaved changes"
            ></i>
          {/if}
        </button>

        <button
          class:active={
            activeSection === 'public'
          }
          type="button"
          aria-pressed={
            activeSection === 'public'
          }
          onclick={() =>
            setSection('public')}
        >
          <span
            class="settings-navigation__icon"
            aria-hidden="true"
          >
            <svg
              viewBox="0 0 24 24"
            >
              <circle
                cx="12"
                cy="12"
                r="9"
              ></circle>

              <path d="M3 12h18"></path>
              <path
                d="M12 3a15 15 0 0 1 0 18"
              ></path>
              <path
                d="M12 3a15 15 0 0 0 0 18"
              ></path>
            </svg>
          </span>

          <span>
            <strong>Public profile</strong>
            <small>
              Username and visibility
            </small>
          </span>

          <span
            class:visibility-indicator={
              publicProfileEnabled
            }
            class="visibility-state"
          >
            {publicProfileEnabled
              ? 'On'
              : 'Off'}
          </span>
        </button>

        <button
          class:active={
            activeSection === 'security'
          }
          type="button"
          aria-pressed={
            activeSection === 'security'
          }
          onclick={() =>
            setSection('security')}
        >
          <span
            class="settings-navigation__icon"
            aria-hidden="true"
          >
            <svg
              viewBox="0 0 24 24"
            >
              <rect
                x="5"
                y="10"
                width="14"
                height="11"
                rx="2"
              ></rect>

              <path
                d="M8 10V7a4 4 0 0 1 8 0v3"
              ></path>
            </svg>
          </span>

          <span>
            <strong>Security</strong>
            <small>
              Change account password
            </small>
          </span>
        </button>

        <button
          class:active={
            activeSection === 'session'
          }
          type="button"
          aria-pressed={
            activeSection === 'session'
          }
          onclick={() =>
            setSection('session')}
        >
          <span
            class="settings-navigation__icon"
            aria-hidden="true"
          >
            <svg
              viewBox="0 0 24 24"
            >
              <rect
                x="3"
                y="4"
                width="18"
                height="16"
                rx="2"
              ></rect>

              <path d="M7 8h10"></path>
              <path d="M7 12h4"></path>
            </svg>
          </span>

          <span>
            <strong>Session</strong>
            <small>
              Account and sign out
            </small>
          </span>

          <i
            class:active={Boolean(session)}
            class="session-dot"
            aria-label={
              session
                ? 'Session active'
                : 'No active session'
            }
          ></i>
        </button>
      </nav>
    </aside>

    <!-- =================================================
         ACCOUNT CONTENT
         ================================================= -->
    <main class="account-content">
      <!-- ===============================================
           PROFILE
           =============================================== -->
      {#if activeSection === 'profile'}
        <section
          class="settings-panel"
          aria-labelledby="profile-title"
        >
          <header class="settings-panel__header">
            <div>
              <p class="panel-kicker">
                Personal identity
              </p>

              <h2 id="profile-title">
                Profile
              </h2>

              <p>
                Keep the information used across
                Softadastra Cloud accurate and
                recognizable.
              </p>
            </div>

            {#if profileChanged}
              <span class="unsaved-label">
                Unsaved changes
              </span>
            {/if}
          </header>

          <form
            class="settings-form"
            onsubmit={handleProfileSubmit}
          >
            <section class="avatar-editor">
              <span
                class="profile-avatar"
                aria-hidden="true"
              >
                {#if avatarUrl}
                  <img
                    src={avatarUrl}
                    alt=""
                  />
                {:else}
                  {initial}
                {/if}
              </span>

              <div class="avatar-editor__content">
                <div>
                  <strong>Profile photo</strong>

                  <p>
                    JPG, PNG or WebP. Maximum file
                    size: 2MB.
                  </p>
                </div>

                <input
                  bind:this={avatarInput}
                  class="avatar-input"
                  type="file"
                  accept="image/jpeg,image/png,image/webp"
                  onchange={handleAvatarChange}
                />

                <div class="avatar-actions">
                  <button
                    type="button"
                    disabled={
                      uploadingAvatar ||
                      deletingAvatar
                    }
                    onclick={openAvatarPicker}
                  >
                    {uploadingAvatar
                      ? 'Uploading…'
                      : avatarUrl
                        ? 'Replace photo'
                        : 'Upload photo'}
                  </button>

                  {#if avatarUrl}
                    <button
                      class="button-secondary"
                      type="button"
                      disabled={
                        uploadingAvatar ||
                        deletingAvatar
                      }
                      onclick={openAvatarRemoval}
                    >
                      Remove
                    </button>
                  {/if}
                </div>
              </div>
            </section>

            <label>
              <span>Display name</span>

              <input
                bind:value={displayName}
                placeholder="Your name"
                autocomplete="name"
                maxlength="80"
                required
              />

              <small>
                Used in your account, workspace
                membership and public profile.
              </small>
            </label>

            <label>
              <span>Bio</span>

              <textarea
                bind:value={bio}
                placeholder="A short description of your work or interests"
                maxlength="500"
                rows="5"
              ></textarea>

              <small class="field-footer">
                <span>
                  Keep it clear and useful to people
                  visiting your profile.
                </span>

                <code>
                  {bio.length}/500
                </code>
              </small>
            </label>

            <div class="field-grid">
              <label>
                <span>Website URL</span>

                <input
                  bind:value={websiteUrl}
                  placeholder="https://example.com"
                  type="url"
                  maxlength="200"
                />

                <small>
                  Personal website, company page or
                  portfolio.
                </small>
              </label>

              <label>
                <span>GitHub URL</span>

                <input
                  bind:value={githubUrl}
                  placeholder="https://github.com/user"
                  type="url"
                  maxlength="200"
                />

                <small>
                  Link to your GitHub account.
                </small>
              </label>
            </div>

            <footer class="form-actions">
              <div>
                <strong>
                  Profile information
                </strong>

                <p>
                  Changes become visible across your
                  account after saving.
                </p>
              </div>

              <button
                type="submit"
                disabled={
                  savingProfile ||
                  !profileChanged
                }
              >
                {savingProfile
                  ? 'Saving profile…'
                  : 'Save profile'}
              </button>
            </footer>
          </form>
        </section>
      {/if}

      <!-- ===============================================
           PUBLIC PROFILE
           =============================================== -->
      {#if activeSection === 'public'}
        <section
          class="settings-panel"
          aria-labelledby="public-profile-title"
        >
          <header class="settings-panel__header">
            <div>
              <p class="panel-kicker">
                Public identity
              </p>

              <h2 id="public-profile-title">
                Public profile
              </h2>

              <p>
                Control the identity and information
                visible outside your private
                workspace.
              </p>
            </div>

            <span
              class:enabled={
                publicProfileEnabled
              }
              class="profile-state"
            >
              {publicProfileEnabled
                ? 'Public'
                : 'Private'}
            </span>
          </header>

          <form
            class="public-profile-layout"
            onsubmit={handleProfileSubmit}
          >
            <div class="public-profile-settings">
              <label>
                <span>Username</span>

                <div class="username-field">
                  <span aria-hidden="true">
                    softadastra.com/u/
                  </span>

                  <input
                    bind:value={username}
                    placeholder="username"
                    autocomplete="username"
                    maxlength="32"
                    pattern="[A-Za-z0-9_-]+"
                  />
                </div>

                <small>
                  Use letters, numbers, hyphens or
                  underscores.
                </small>
              </label>

              <div class="visibility-control">
                <div>
                  <strong>
                    Public profile visibility
                  </strong>

                  <p>
                    Allow people to visit your profile
                    without signing in.
                  </p>
                </div>

                <button
                  class:enabled={
                    publicProfileEnabled
                  }
                  class="switch-control"
                  type="button"
                  role="switch"
                  aria-checked={
                    publicProfileEnabled
                  }
                  onclick={() => {
                    publicProfileEnabled =
                      !publicProfileEnabled;
                  }}
                >
                  <span></span>

                  <strong>
                    {publicProfileEnabled
                      ? 'Enabled'
                      : 'Disabled'}
                  </strong>
                </button>
              </div>

              <div class="visibility-explanation">
                <span aria-hidden="true">
                  <svg
                    viewBox="0 0 24 24"
                    fill="none"
                    stroke="currentColor"
                    stroke-width="1.8"
                    stroke-linecap="round"
                    stroke-linejoin="round"
                  >
                    <path
                      d="M2 12s3.5-6 10-6 10 6 10 6-3.5 6-10 6S2 12 2 12Z"
                    ></path>

                    <circle
                      cx="12"
                      cy="12"
                      r="2.5"
                    ></circle>
                  </svg>
                </span>

                <div>
                  <strong>
                    Public information
                  </strong>

                  <p>
                    Your avatar, display name,
                    username, bio, links and public
                    package activity may be shown.
                  </p>
                </div>
              </div>

              <div class="privacy-boundary">
                <span aria-hidden="true">
                  <svg
                    viewBox="0 0 24 24"
                    fill="none"
                    stroke="currentColor"
                    stroke-width="1.8"
                    stroke-linecap="round"
                    stroke-linejoin="round"
                  >
                    <rect
                      x="5"
                      y="10"
                      width="14"
                      height="11"
                      rx="2"
                    ></rect>

                    <path
                      d="M8 10V7a4 4 0 0 1 8 0v3"
                    ></path>
                  </svg>
                </span>

                <div>
                  <strong>
                    Always private
                  </strong>

                  <p>
                    Private workspaces, private
                    packages, lockfiles, build
                    reports, members, tokens,
                    notifications and feedback are
                    never displayed.
                  </p>
                </div>
              </div>
            </div>

            <aside class="profile-preview">
              <header>
                <span>Public preview</span>

                <span
                  class:online={
                    publicProfileEnabled
                  }
                  class="preview-status"
                >
                  {publicProfileEnabled
                    ? 'Visible'
                    : 'Hidden'}
                </span>
              </header>

              <div class="profile-preview__body">
                <span
                  class="preview-avatar"
                  aria-hidden="true"
                >
                  {#if avatarUrl}
                    <img
                      src={avatarUrl}
                      alt=""
                    />
                  {:else}
                    {initial}
                  {/if}
                </span>

                <div class="preview-identity">
                  <h3>{accountName}</h3>

                  <span>
                    {normalizedUsername
                      ? `@${normalizedUsername}`
                      : '@username'}
                  </span>
                </div>

                <p class:placeholder={!normalizedBio}>
                  {normalizedBio ||
                    'Your profile bio will appear here.'}
                </p>

                <div class="preview-links">
                  {#if normalizedWebsiteUrl}
                    <span>
                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <circle
                          cx="12"
                          cy="12"
                          r="9"
                        ></circle>

                        <path d="M3 12h18"></path>
                      </svg>

                      Website
                    </span>
                  {/if}

                  {#if normalizedGithubUrl}
                    <span>
                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path
                          d="M15 22v-4a4.8 4.8 0 0 0-1-3.5c3.3-.4 6.8-1.6 6.8-7A5.4 5.4 0 0 0 19.4 4 5 5 0 0 0 19.3.5S18.2.1 15 1.8a13.4 13.4 0 0 0-7 0C4.8.1 3.7.5 3.7.5A5 5 0 0 0 3.6 4a5.4 5.4 0 0 0-1.4 3.7c0 5.4 3.5 6.6 6.8 7A4.8 4.8 0 0 0 8 18v4"
                        ></path>
                      </svg>

                      GitHub
                    </span>
                  {/if}

                  {#if
                    !normalizedWebsiteUrl &&
                    !normalizedGithubUrl
                  }
                    <span class="placeholder-link">
                      No public links yet
                    </span>
                  {/if}
                </div>
              </div>

              <footer>
                <span>
                  softadastra.com/u/
                </span>

                <strong>
                  {normalizedUsername ||
                    'username'}
                </strong>
              </footer>
            </aside>

            <footer class="form-actions public-actions">
              <div>
                {#if canViewPublicProfile}
                  <a
                    class="secondary-link"
                    href={savedPublicProfileUrl}
                  >
                    View current profile

                    <svg
                      viewBox="0 0 24 24"
                      aria-hidden="true"
                    >
                      <path
                        d="M14 3h7v7"
                      ></path>

                      <path
                        d="M10 14 21 3"
                      ></path>

                      <path
                        d="M21 14v5a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5"
                      ></path>
                    </svg>
                  </a>

                  <a
                    class="secondary-link"
                    href={`${savedPublicProfileUrl}?pins=1`}
                  >
                    Customize pins
                  </a>
                {:else}
                  <p>
                    Save a username and enable
                    visibility to publish the profile.
                  </p>
                {/if}
              </div>

              <button
                type="submit"
                disabled={
                  savingProfile ||
                  !profileChanged
                }
              >
                {savingProfile
                  ? 'Saving visibility…'
                  : 'Save public profile'}
              </button>
            </footer>
          </form>
        </section>
      {/if}

      <!-- ===============================================
           SECURITY
           =============================================== -->
      {#if activeSection === 'security'}
        <section
          class="settings-panel"
          aria-labelledby="security-title"
        >
          <header class="settings-panel__header">
            <div>
              <p class="panel-kicker">
                Account protection
              </p>

              <h2 id="security-title">
                Password
              </h2>

              <p>
                Change the password used to access
                your Softadastra Cloud account.
              </p>
            </div>

            <span
              class="security-indicator"
              aria-hidden="true"
            >
              <svg
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="1.8"
                stroke-linecap="round"
                stroke-linejoin="round"
              >
                <path
                  d="M12 3 4 7v5c0 5 3.4 8.7 8 10 4.6-1.3 8-5 8-10V7l-8-4Z"
                ></path>

                <path d="m9 12 2 2 4-4"></path>
              </svg>
            </span>
          </header>

          <form
            class="settings-form password-form"
            onsubmit={handlePasswordSubmit}
          >
            <label>
              <span>Current password</span>

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
                  onclick={() => {
                    showCurrentPassword =
                      !showCurrentPassword;
                  }}
                >
                  {#if showCurrentPassword}
                    <svg
                      viewBox="0 0 24 24"
                      aria-hidden="true"
                    >
                      <path
                        d="M3 3l18 18"
                      ></path>

                      <path
                        d="M10.6 10.6a2 2 0 0 0 2.8 2.8"
                      ></path>

                      <path
                        d="M9.9 4.2A10.8 10.8 0 0 1 12 4c6.5 0 10 8 10 8a17.5 17.5 0 0 1-2.2 3.4"
                      ></path>

                      <path
                        d="M6.6 6.6C3.7 8.5 2 12 2 12s3.5 8 10 8a9.7 9.7 0 0 0 5.4-1.6"
                      ></path>
                    </svg>
                  {:else}
                    <svg
                      viewBox="0 0 24 24"
                      aria-hidden="true"
                    >
                      <path
                        d="M2 12s3.5-8 10-8 10 8 10 8-3.5 8-10 8S2 12 2 12Z"
                      ></path>

                      <circle
                        cx="12"
                        cy="12"
                        r="2.5"
                      ></circle>
                    </svg>
                  {/if}
                </button>
              </span>
            </label>

            <div class="field-grid">
              <label>
                <span>New password</span>

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
                    onclick={() => {
                      showNewPassword =
                        !showNewPassword;
                    }}
                  >
                    {#if showNewPassword}
                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path
                          d="M3 3l18 18"
                        ></path>

                        <path
                          d="M10.6 10.6a2 2 0 0 0 2.8 2.8"
                        ></path>

                        <path
                          d="M9.9 4.2A10.8 10.8 0 0 1 12 4c6.5 0 10 8 10 8a17.5 17.5 0 0 1-2.2 3.4"
                        ></path>
                      </svg>
                    {:else}
                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path
                          d="M2 12s3.5-8 10-8 10 8 10 8-3.5 8-10 8S2 12 2 12Z"
                        ></path>

                        <circle
                          cx="12"
                          cy="12"
                          r="2.5"
                        ></circle>
                      </svg>
                    {/if}
                  </button>
                </span>
              </label>

              <label>
                <span>Confirm new password</span>

                <span class="password-field">
                  <input
                    class:invalid={
                      Boolean(
                        confirmNewPassword
                      ) &&
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
                      Boolean(
                        confirmNewPassword
                      ) &&
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
                    onclick={() => {
                      showConfirmPassword =
                        !showConfirmPassword;
                    }}
                  >
                    {#if showConfirmPassword}
                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path
                          d="M3 3l18 18"
                        ></path>

                        <path
                          d="M10.6 10.6a2 2 0 0 0 2.8 2.8"
                        ></path>
                      </svg>
                    {:else}
                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path
                          d="M2 12s3.5-8 10-8 10 8 10 8-3.5 8-10 8S2 12 2 12Z"
                        ></path>

                        <circle
                          cx="12"
                          cy="12"
                          r="2.5"
                        ></circle>
                      </svg>
                    {/if}
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

            <section class="password-strength">
              <header>
                <span>Password strength</span>

                <strong
                  class:weak={
                    passwordStrength === 1
                  }
                  class:fair={
                    passwordStrength === 2
                  }
                  class:good={
                    passwordStrength === 3
                  }
                  class:strong={
                    passwordStrength === 4
                  }
                >
                  {passwordStrengthLabel}
                </strong>
              </header>

              <div
                class="strength-bars"
                aria-hidden="true"
              >
                {#each [1, 2, 3, 4] as level}
                  <span
                    class:active={
                      passwordStrength >= level
                    }
                    class:weak={
                      passwordStrength === 1
                    }
                    class:fair={
                      passwordStrength === 2
                    }
                    class:good={
                      passwordStrength === 3
                    }
                    class:strong={
                      passwordStrength === 4
                    }
                  ></span>
                {/each}
              </div>

              <div class="password-guidance">
                <span>
                  <i
                    class:complete={
                      newPassword.length >= 8
                    }
                    aria-hidden="true"
                  ></i>

                  Use a longer password
                </span>

                <span>
                  <i
                    class:complete={
                      /[a-z]/.test(newPassword) &&
                      /[A-Z]/.test(newPassword)
                    }
                    aria-hidden="true"
                  ></i>

                  Mix uppercase and lowercase
                </span>

                <span>
                  <i
                    class:complete={
                      /\d/.test(newPassword) &&
                      /[^A-Za-z0-9]/.test(
                        newPassword
                      )
                    }
                    aria-hidden="true"
                  ></i>

                  Add numbers and symbols
                </span>
              </div>
            </section>

            <div class="security-note">
              <span aria-hidden="true">
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                >
                  <rect
                    x="5"
                    y="10"
                    width="14"
                    height="11"
                    rx="2"
                  ></rect>

                  <path
                    d="M8 10V7a4 4 0 0 1 8 0v3"
                  ></path>
                </svg>
              </span>

              <p>
                Use a password that is not shared
                with another service. Passwords are
                never displayed or stored in plain
                text.
              </p>
            </div>

            <footer class="form-actions">
              <div>
                <strong>
                  Account password
                </strong>

                <p>
                  You will use the new password the
                  next time authentication is
                  required.
                </p>
              </div>

              <button
                type="submit"
                disabled={
                  savingPassword ||
                  !canChangePassword
                }
              >
                {savingPassword
                  ? 'Changing password…'
                  : 'Change password'}
              </button>
            </footer>
          </form>
        </section>
      {/if}

      <!-- ===============================================
           SESSION
           =============================================== -->
      {#if activeSection === 'session'}
        <section
          class="settings-panel"
          aria-labelledby="session-title"
        >
          <header class="settings-panel__header">
            <div>
              <p class="panel-kicker">
                Account access
              </p>

              <h2 id="session-title">
                Current session
              </h2>

              <p>
                Review the account identity and
                authentication session currently
                active in this browser.
              </p>
            </div>

            <span
              class:active={
                Boolean(session)
              }
              class="session-status"
            >
              <i aria-hidden="true"></i>

              {session
                ? 'Active'
                : 'Inactive'}
            </span>
          </header>

          <div class="session-overview">
            <article>
              <span>Email address</span>

              <strong>
                {user?.email ?? 'Unknown'}
              </strong>

              <small>
                Primary account identity
              </small>
            </article>

            <article>
              <span>Display name</span>

              <strong>
                {user?.display_name ||
                  user?.name ||
                  'Not set'}
              </strong>

              <small>
                Visible across Cloud
              </small>
            </article>

            <article>
              <span>Session expires</span>

              <strong>
                {formatDate(
                  session?.expires_at
                )}
              </strong>

              <small>
                Current browser session
              </small>
            </article>
          </div>

          <section class="account-identifier">
            <div>
              <p class="panel-kicker">
                Account identifier
              </p>

              <h3>User ID</h3>

              <p>
                Use this identifier when support or
                account diagnostics require an exact
                account reference.
              </p>
            </div>

            <div class="user-id-field">
              <code title={user?.id}>
                {user?.id ?? 'Unknown'}
              </code>

              <button
                class:copied={copiedUserId}
                class="copy-button"
                type="button"
                disabled={!user?.id}
                aria-label={
                  copiedUserId
                    ? 'User ID copied'
                    : 'Copy user ID'
                }
                onclick={copyUserId}
              >
                {#if copiedUserId}
                  <svg
                    viewBox="0 0 24 24"
                    aria-hidden="true"
                  >
                    <path
                      d="m5 12 4 4L19 6"
                    ></path>
                  </svg>

                  Copied
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

                  Copy
                {/if}
              </button>
            </div>
          </section>

          {#if
            loadingSupporterStatus ||
            supporterStatus
          }
            <section class="supporter-record">
              <header>
                <div>
                  <p class="panel-kicker">
                    Account recognition
                  </p>

                  <h3>
                    Founding Supporter
                  </h3>
                </div>

                {#if supporterStatus}
                  <SupporterBadge
                    tier={supporterStatus.tier}
                  />
                {/if}
              </header>

              {#if loadingSupporterStatus}
                <p class="supporter-loading">
                  Loading supporter record…
                </p>
              {:else if supporterStatus}
                <dl>
                  <div>
                    <dt>Tier</dt>

                    <dd>
                      {supporterStatus.tier ===
                      'founding_builder'
                        ? 'Founding Builder'
                        : 'Founding Supporter'}
                    </dd>
                  </div>

                  <div>
                    <dt>Status</dt>

                    <dd>
                      {supporterStatus.status}
                    </dd>
                  </div>

                  <div>
                    <dt>Since</dt>

                    <dd>
                      {formatDate(
                        supporterStatus.started_at
                      )}
                    </dd>
                  </div>

                  <div>
                    <dt>Public visibility</dt>

                    <dd>
                      {supporterStatus.public_visible
                        ? 'Visible'
                        : 'Hidden'}
                    </dd>
                  </div>
                </dl>
              {/if}
            </section>
          {/if}

          <section class="sign-out-section">
            <div>
              <span
                class="sign-out-section__icon"
                aria-hidden="true"
              >
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                >
                  <path
                    d="M10 17l5-5-5-5"
                  ></path>

                  <path
                    d="M15 12H3"
                  ></path>

                  <path
                    d="M15 3h4a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2h-4"
                  ></path>
                </svg>
              </span>

              <div>
                <strong>
                  Sign out of this browser
                </strong>

                <p>
                  The current session will end and
                  you will need to authenticate
                  again.
                </p>
              </div>
            </div>

            <button
              class="sign-out-button"
              type="button"
              onclick={logout}
            >
              Sign out
            </button>
          </section>
        </section>
      {/if}
    </main>
  </div>
</div>

<!-- =====================================================
     REMOVE AVATAR CONFIRMATION
     ===================================================== -->
{#if showAvatarRemoval}
  <div class="modal-layer">
    <button
      class="modal-backdrop"
      type="button"
      aria-label="Close confirmation"
      onclick={closeAvatarRemoval}
    ></button>

    <dialog
      bind:this={avatarRemovalDialog}
      class="confirm-modal"
      open
      aria-modal="true"
      aria-labelledby="avatar-removal-title"
      aria-describedby="avatar-removal-description"
      tabindex="-1"
    >
      <span
        class="confirm-modal__icon"
        aria-hidden="true"
      >
        <svg
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="1.9"
          stroke-linecap="round"
          stroke-linejoin="round"
        >
          <path d="M3 6h18"></path>
          <path d="M8 6V4h8v2"></path>
          <path d="M6 6l1 15h10l1-15"></path>
          <path d="M10 11v5"></path>
          <path d="M14 11v5"></path>
        </svg>
      </span>

      <div class="confirm-modal__content">
        <h2 id="avatar-removal-title">
          Remove profile photo?
        </h2>

        <p id="avatar-removal-description">
          Your account will return to the initial
          generated from your display name.
        </p>
      </div>

      <div class="confirm-modal__actions">
        <button
          class="button-secondary"
          type="button"
          disabled={deletingAvatar}
          onclick={closeAvatarRemoval}
        >
          Cancel
        </button>

        <button
          class="danger-button"
          type="button"
          disabled={deletingAvatar}
          onclick={confirmAvatarRemoval}
        >
          {deletingAvatar
            ? 'Removing photo…'
            : 'Remove photo'}
        </button>
      </div>
    </dialog>
  </div>
{/if}

<style>
  .account-page {
    display: grid;
    max-width: 1180px;
    gap: 18px;
    margin-inline: auto;
    padding-bottom: 44px;
    animation: page-enter 420ms var(--ease) both;
  }

  @keyframes page-enter {
    from {
      opacity: 0;
      transform: translateY(7px);
    }
  }

  /* =====================================================
     PAGE HEADER
     ===================================================== */

  .page-header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 24px;
    padding-bottom: 22px;
    border-bottom: 1px solid var(--line);
  }

  .page-heading {
    display: grid;
    gap: 7px;
  }

  .page-eyebrow {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    color: var(--brand-bright);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.1em;
    text-transform: uppercase;
  }

  .page-eyebrow > span {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--brand);
    box-shadow: 0 0 0 4px var(--brand-faint);
  }

  .page-heading h1 {
    color: var(--text);
    font-size: clamp(30px, 4vw, 44px);
    font-weight: 650;
    line-height: 1;
    letter-spacing: -0.045em;
  }

  .page-heading > p:last-child {
    max-width: 680px;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.6;
  }

  .header-account {
    display: flex;
    min-width: 230px;
    align-items: center;
    gap: 10px;
    padding: 7px 11px;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
    background: var(--bg-panel);
  }

  .header-account__avatar {
    display: grid;
    width: 34px;
    height: 34px;
    flex: 0 0 auto;
    place-items: center;
    overflow: hidden;
    border: 1px solid var(--brand-line);
    border-radius: 50%;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 11px;
    font-weight: 700;
  }

  .header-account__avatar img,
  .identity-avatar img,
  .profile-avatar img,
  .preview-avatar img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }

  .header-account > span:last-child {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .header-account strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 10px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .header-account small {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 8.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  /* =====================================================
     SHARED CONTROLS
     ===================================================== */

  button {
    display: inline-flex;
    min-height: 39px;
    align-items: center;
    justify-content: center;
    gap: 7px;
    border: 1px solid var(--brand);
    border-radius: var(--radius-sm);
    background: var(--brand);
    color: var(--brand-ink);
    padding: 0 14px;
    font: inherit;
    font-size: 12px;
    font-weight: 650;
    cursor: pointer;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      color var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  button:hover:not(:disabled) {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
    transform: translateY(-1px);
  }

  button:disabled {
    cursor: not-allowed;
    opacity: 0.5;
  }

  .button-secondary {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .button-secondary:hover:not(:disabled) {
    border-color: rgba(255, 255, 255, 0.17);
    background: var(--bg-elevated);
    color: var(--text);
  }

  .danger-button,
  .sign-out-button {
    border-color: var(--danger-line);
    background: transparent;
    color: var(--danger);
  }

  .danger-button:hover:not(:disabled),
  .sign-out-button:hover:not(:disabled) {
    border-color: var(--danger);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .panel-kicker {
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 9px;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  /* =====================================================
     SUCCESS
     ===================================================== */

  .success-message {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 11px 13px;
    border: 1px solid var(--green-line);
    border-radius: var(--radius-sm);
    background: var(--green-faint);
  }

  .success-message > span {
    display: grid;
    width: 27px;
    height: 27px;
    flex: 0 0 auto;
    place-items: center;
    border-radius: 7px;
    color: var(--green-soft);
  }

  .success-message svg {
    width: 14px;
    height: 14px;
  }

  .success-message p {
    color: var(--green-soft);
    font-size: 12px;
  }

  /* =====================================================
     ACCOUNT LAYOUT
     ===================================================== */

  .account-shell {
    display: grid;
    grid-template-columns:
      270px minmax(0, 1fr);
    gap: 16px;
    align-items: start;
  }

  .account-navigation {
    position: sticky;
    top: 24px;
    display: grid;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .identity-card {
    display: grid;
    gap: 14px;
    padding: 16px;
    border-bottom: 1px solid var(--line-soft);
  }

  .identity-card__main {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 11px;
  }

  .identity-avatar {
    display: grid;
    width: 45px;
    height: 45px;
    flex: 0 0 auto;
    place-items: center;
    overflow: hidden;
    border: 1px solid var(--brand-line);
    border-radius: 50%;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 14px;
    font-weight: 700;
  }

  .identity-card__main > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .identity-card__main strong {
    overflow: hidden;
    color: var(--text);
    font-size: 12px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .identity-card__main div > span {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 9px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .identity-supporter {
    display: flex;
  }

  .completion {
    display: grid;
    gap: 7px;
  }

  .completion__heading {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 10px;
  }

  .completion__heading span {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .completion__heading strong {
    color: var(--text-soft);
    font-family: var(--font-mono);
    font-size: 8.5px;
  }

  .completion__track {
    height: 3px;
    overflow: hidden;
    border-radius: 999px;
    background: var(--line-strong);
  }

  .completion__track > span {
    display: block;
    height: 100%;
    border-radius: inherit;
    background: var(--brand);
    transition: width 300ms var(--ease);
  }

  .settings-navigation {
    display: grid;
    padding: 7px;
  }

  .settings-navigation button {
    display: grid;
    width: 100%;
    min-height: 59px;
    grid-template-columns:
      30px minmax(0, 1fr) auto;
    align-items: center;
    justify-content: stretch;
    gap: 9px;
    border: 0;
    border-radius: 7px;
    background: transparent;
    color: var(--text);
    padding: 8px 9px;
    text-align: left;
  }

  .settings-navigation button:hover:not(:disabled) {
    background: var(--bg-elevated);
    transform: none;
  }

  .settings-navigation button.active {
    background: var(--brand-faint);
    box-shadow: inset 2px 0 0 var(--brand);
  }

  .settings-navigation__icon {
    display: grid;
    width: 29px;
    height: 29px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 7px;
    color: var(--text-muted);
  }

  .settings-navigation button.active
    .settings-navigation__icon {
    border-color: var(--brand-line);
    color: var(--brand-soft);
  }

  .settings-navigation__icon svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .settings-navigation button > span:nth-child(2) {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .settings-navigation button strong {
    color: var(--text-soft);
    font-size: 10.5px;
  }

  .settings-navigation button.active strong {
    color: var(--brand-bright);
  }

  .settings-navigation button small {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 8px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .unsaved-dot,
  .session-dot {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: var(--brand);
  }

  .session-dot {
    background: var(--text-faint);
  }

  .session-dot.active {
    background: var(--green-soft);
  }

  .visibility-state {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    font-weight: 700;
    text-transform: uppercase;
  }

  .visibility-state.visibility-indicator {
    color: var(--brand-soft);
  }

  .account-content {
    min-width: 0;
  }

  /* =====================================================
     SETTINGS PANEL
     ===================================================== */

  .settings-panel {
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    animation: panel-enter 260ms var(--ease) both;
  }

  @keyframes panel-enter {
    from {
      opacity: 0;
      transform: translateY(5px);
    }
  }

  .settings-panel__header {
    display: flex;
    min-height: 95px;
    align-items: flex-start;
    justify-content: space-between;
    gap: 18px;
    padding: 18px 19px;
    border-bottom: 1px solid var(--line-soft);
  }

  .settings-panel__header > div {
    display: grid;
    gap: 4px;
  }

  .settings-panel__header h2 {
    color: var(--text);
    font-size: 19px;
    font-weight: 620;
    letter-spacing: -0.025em;
  }

  .settings-panel__header p:not(.panel-kicker) {
    max-width: 660px;
    color: var(--text-muted);
    font-size: 10.5px;
    line-height: 1.55;
  }

  .unsaved-label,
  .profile-state,
  .session-status {
    display: inline-flex;
    min-height: 25px;
    flex: 0 0 auto;
    align-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 999px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    padding: 0 9px;
    font-size: 8.5px;
    font-weight: 600;
  }

  .profile-state {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-muted);
  }

  .profile-state.enabled {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .security-indicator {
    display: grid;
    width: 40px;
    height: 40px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--green-line);
    border-radius: 9px;
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .security-indicator svg {
    width: 19px;
    height: 19px;
  }

  /* =====================================================
     FORMS
     ===================================================== */

  .settings-form {
    display: grid;
    gap: 18px;
    padding: 20px;
  }

  .settings-form label,
  .public-profile-settings label {
    display: grid;
    gap: 7px;
  }

  .settings-form label > span:first-child,
  .public-profile-settings label > span:first-child {
    color: var(--text-soft);
    font-size: 10.5px;
    font-weight: 550;
  }

  .settings-form input,
  .settings-form textarea,
  .public-profile-settings input {
    width: 100%;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    outline: 0;
    background: transparent;
    color: var(--text);
    padding-inline: 11px;
    font: inherit;
    font-size: 12px;
  }

  .settings-form input,
  .public-profile-settings input {
    min-height: 42px;
  }

  .settings-form textarea {
    min-height: 135px;
    resize: vertical;
    padding-block: 11px;
    line-height: 1.65;
  }

  .settings-form input:focus,
  .settings-form textarea:focus,
  .public-profile-settings input:focus {
    border-color: var(--brand);
    box-shadow: 0 0 0 3px var(--brand-faint);
  }

  .settings-form label > small,
  .public-profile-settings label > small {
    color: var(--text-muted);
    font-size: 9px;
    line-height: 1.5;
  }

  .field-footer {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 14px;
  }

  .field-footer code {
    flex: 0 0 auto;
    color: var(--text-faint);
    font-size: 8.5px;
  }

  .field-grid {
    display: grid;
    grid-template-columns:
      repeat(2, minmax(0, 1fr));
    gap: 14px;
  }

  .form-actions {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    margin: 3px -20px -20px;
    padding: 14px 20px;
    border-top: 1px solid var(--line-soft);
  }

  .form-actions > div {
    display: grid;
    gap: 3px;
  }

  .form-actions > div strong {
    color: var(--text-soft);
    font-size: 9.5px;
  }

  .form-actions > div p {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  /* =====================================================
     AVATAR
     ===================================================== */

  .avatar-editor {
    display: grid;
    grid-template-columns:
      76px minmax(0, 1fr);
    align-items: center;
    gap: 15px;
    padding-bottom: 19px;
    border-bottom: 1px solid var(--line-soft);
  }

  .profile-avatar {
    display: grid;
    width: 70px;
    height: 70px;
    place-items: center;
    overflow: hidden;
    border: 1px solid var(--brand-line);
    border-radius: 50%;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 22px;
    font-weight: 700;
  }

  .avatar-editor__content {
    display: grid;
    min-width: 0;
    gap: 11px;
  }

  .avatar-editor__content > div:first-child {
    display: grid;
    gap: 3px;
  }

  .avatar-editor__content strong {
    color: var(--text);
    font-size: 11px;
  }

  .avatar-editor__content p {
    color: var(--text-muted);
    font-size: 9px;
  }

  .avatar-input {
    display: none;
  }

  .avatar-actions {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
  }

  .avatar-actions button {
    min-height: 34px;
    padding-inline: 11px;
    font-size: 9.5px;
  }

  /* =====================================================
     PUBLIC PROFILE
     ===================================================== */

  .public-profile-layout {
    display: grid;
    grid-template-columns:
      minmax(0, 1.1fr)
      minmax(300px, 0.9fr);
  }

  .public-profile-settings {
    display: grid;
    align-content: start;
    gap: 17px;
    padding: 20px;
    border-right: 1px solid var(--line-soft);
  }

  .username-field {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr);
    align-items: center;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
  }

  .username-field:focus-within {
    border-color: var(--brand);
    box-shadow: 0 0 0 3px var(--brand-faint);
  }

  .username-field > span {
    display: flex;
    min-height: 40px;
    align-items: center;
    padding: 0 0 0 11px;
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 9px;
  }

  .username-field input {
    min-height: 40px;
    border: 0;
    border-radius: 0;
    padding-left: 3px;
  }

  .username-field input:focus {
    box-shadow: none;
  }

  .visibility-control {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    padding: 13px;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
  }

  .visibility-control > div {
    display: grid;
    gap: 4px;
  }

  .visibility-control strong {
    color: var(--text-soft);
    font-size: 10.5px;
  }

  .visibility-control p {
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.5;
  }

  .switch-control {
    display: grid;
    min-width: 96px;
    min-height: 34px;
    grid-template-columns:
      28px minmax(0, 1fr);
    align-items: center;
    gap: 6px;
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-muted);
    padding: 2px 8px 2px 3px;
  }

  .switch-control:hover:not(:disabled) {
    border-color: var(--line-strong);
    background: var(--bg-elevated);
    color: var(--text-soft);
    transform: none;
  }

  .switch-control > span {
    position: relative;
    width: 26px;
    height: 18px;
    border-radius: 999px;
    background: var(--line-strong);
  }

  .switch-control > span::after {
    content: '';
    position: absolute;
    top: 3px;
    left: 3px;
    width: 12px;
    height: 12px;
    border-radius: 50%;
    background: var(--text-muted);
    transition:
      transform var(--speed) var(--ease),
      background var(--speed) var(--ease);
  }

  .switch-control.enabled {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .switch-control.enabled > span {
    background: var(--brand);
  }

  .switch-control.enabled > span::after {
    background: var(--brand-ink);
    transform: translateX(8px);
  }

  .switch-control strong {
    font-size: 8px;
  }

  .visibility-explanation,
  .privacy-boundary {
    display: grid;
    grid-template-columns:
      32px minmax(0, 1fr);
    gap: 10px;
    padding: 12px;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
  }

  .visibility-explanation > span,
  .privacy-boundary > span {
    display: grid;
    width: 31px;
    height: 31px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 7px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .privacy-boundary > span {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-muted);
  }

  .visibility-explanation svg,
  .privacy-boundary svg {
    width: 15px;
    height: 15px;
  }

  .visibility-explanation > div,
  .privacy-boundary > div {
    display: grid;
    gap: 4px;
  }

  .visibility-explanation strong,
  .privacy-boundary strong {
    color: var(--text-soft);
    font-size: 9.5px;
  }

  .visibility-explanation p,
  .privacy-boundary p {
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.55;
  }

  .profile-preview {
    display: grid;
    align-content: start;
    border-left: 0;
  }

  .profile-preview > header {
    display: flex;
    min-height: 46px;
    align-items: center;
    justify-content: space-between;
    gap: 10px;
    padding: 0 14px;
    border-bottom: 1px solid var(--line-soft);
  }

  .profile-preview > header > span:first-child {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 8px;
    text-transform: uppercase;
  }

  .preview-status {
    display: inline-flex;
    min-height: 22px;
    align-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    color: var(--text-muted);
    padding: 0 8px;
    font-size: 8px;
  }

  .preview-status.online {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .profile-preview__body {
    display: grid;
    justify-items: start;
    gap: 12px;
    min-height: 330px;
    align-content: start;
    padding: 24px 19px;
  }

  .preview-avatar {
    display: grid;
    width: 66px;
    height: 66px;
    place-items: center;
    overflow: hidden;
    border: 1px solid var(--brand-line);
    border-radius: 50%;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 21px;
    font-weight: 700;
  }

  .preview-identity {
    display: grid;
    gap: 3px;
  }

  .preview-identity h3 {
    color: var(--text);
    font-size: 18px;
    font-weight: 620;
  }

  .preview-identity span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 9px;
  }

  .profile-preview__body > p {
    max-width: 410px;
    color: var(--text-soft);
    font-size: 10px;
    line-height: 1.65;
  }

  .profile-preview__body > p.placeholder {
    color: var(--text-muted);
  }

  .preview-links {
    display: flex;
    flex-wrap: wrap;
    gap: 7px;
  }

  .preview-links > span {
    display: inline-flex;
    min-height: 27px;
    align-items: center;
    gap: 5px;
    border: 1px solid var(--line);
    border-radius: 999px;
    color: var(--text-muted);
    padding: 0 8px;
    font-size: 8px;
  }

  .preview-links svg {
    width: 11px;
    height: 11px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .placeholder-link {
    border-style: dashed !important;
  }

  .profile-preview > footer {
    display: flex;
    min-height: 43px;
    align-items: center;
    gap: 2px;
    padding: 0 14px;
    border-top: 1px solid var(--line-soft);
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 8px;
  }

  .profile-preview > footer strong {
    color: var(--brand-soft);
    font-weight: 600;
  }

  .public-actions {
    grid-column: 1 / -1;
    margin: 0;
  }

  .public-actions > div {
    display: flex;
    align-items: center;
    gap: 13px;
  }

  .public-actions > div > p {
    max-width: 460px;
  }

  .secondary-link {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    color: var(--brand-soft);
    font-size: 9px;
    font-weight: 600;
    text-decoration: none;
  }

  .secondary-link:hover {
    color: var(--brand-bright);
  }

  .secondary-link svg {
    width: 12px;
    height: 12px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  /* =====================================================
     PASSWORD
     ===================================================== */

  .password-field {
    position: relative;
    display: block;
  }

  .password-field input {
    padding-right: 46px;
  }

  .password-field input.invalid {
    border-color: var(--danger);
  }

  .password-toggle {
    position: absolute;
    top: 50%;
    right: 5px;
    display: grid;
    width: 31px;
    height: 31px;
    min-height: 31px;
    place-items: center;
    border: 0;
    background: transparent;
    color: var(--text-muted);
    padding: 0;
    transform: translateY(-50%);
  }

  .password-toggle:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
    transform: translateY(-50%);
  }

  .password-toggle svg {
    width: 15px;
    height: 15px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .password-error {
    color: var(--danger) !important;
  }

  .password-strength {
    display: grid;
    gap: 10px;
    padding: 13px;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
  }

  .password-strength > header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
  }

  .password-strength header span {
    color: var(--text-muted);
    font-size: 9px;
  }

  .password-strength header strong {
    color: var(--text-muted);
    font-size: 9px;
  }

  .password-strength header strong.weak {
    color: var(--danger);
  }

  .password-strength header strong.fair {
    color: var(--warning);
  }

  .password-strength header strong.good,
  .password-strength header strong.strong {
    color: var(--green-soft);
  }

  .strength-bars {
    display: grid;
    grid-template-columns:
      repeat(4, minmax(0, 1fr));
    gap: 5px;
  }

  .strength-bars span {
    height: 3px;
    border-radius: 999px;
    background: var(--line-strong);
  }

  .strength-bars span.active.weak {
    background: var(--danger);
  }

  .strength-bars span.active.fair {
    background: var(--warning);
  }

  .strength-bars span.active.good,
  .strength-bars span.active.strong {
    background: var(--green-soft);
  }

  .password-guidance {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    gap: 9px;
  }

  .password-guidance > span {
    display: flex;
    align-items: center;
    gap: 6px;
    color: var(--text-muted);
    font-size: 8px;
  }

  .password-guidance i {
    width: 7px;
    height: 7px;
    flex: 0 0 auto;
    border-radius: 50%;
    background: var(--line-strong);
  }

  .password-guidance i.complete {
    background: var(--green-soft);
  }

  .security-note {
    display: grid;
    grid-template-columns:
      32px minmax(0, 1fr);
    align-items: center;
    gap: 10px;
    padding: 12px;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
  }

  .security-note > span {
    display: grid;
    width: 31px;
    height: 31px;
    place-items: center;
    border: 1px solid var(--green-line);
    border-radius: 7px;
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .security-note svg {
    width: 15px;
    height: 15px;
  }

  .security-note p {
    color: var(--text-muted);
    font-size: 9px;
    line-height: 1.55;
  }

  /* =====================================================
     SESSION
     ===================================================== */

  .session-status {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-muted);
  }

  .session-status i {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--text-faint);
  }

  .session-status.active {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .session-status.active i {
    background: var(--green-soft);
  }

  .session-overview {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    border-bottom: 1px solid var(--line-soft);
  }

  .session-overview article {
    display: grid;
    min-width: 0;
    gap: 5px;
    padding: 17px 18px;
    border-right: 1px solid var(--line-soft);
  }

  .session-overview article:last-child {
    border-right: 0;
  }

  .session-overview span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7.5px;
    text-transform: uppercase;
  }

  .session-overview strong {
    overflow-wrap: anywhere;
    color: var(--text);
    font-size: 11px;
    font-weight: 600;
  }

  .session-overview small {
    color: var(--text-faint);
    font-size: 8px;
  }

  .account-identifier {
    display: grid;
    grid-template-columns:
      minmax(0, 0.75fr)
      minmax(300px, 1.25fr);
    align-items: center;
    gap: 30px;
    padding: 19px;
    border-bottom: 1px solid var(--line-soft);
  }

  .account-identifier > div:first-child {
    display: grid;
    gap: 4px;
  }

  .account-identifier h3,
  .supporter-record h3 {
    color: var(--text);
    font-size: 13px;
    font-weight: 600;
  }

  .account-identifier p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 9px;
    line-height: 1.55;
  }

  .user-id-field {
    display: grid;
    grid-template-columns:
      minmax(0, 1fr) auto;
    align-items: center;
    gap: 8px;
    padding: 6px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
  }

  .user-id-field code {
    min-width: 0;
    overflow: hidden;
    color: var(--text-soft);
    padding-left: 7px;
    font-size: 9px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .copy-button {
    min-height: 31px;
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-muted);
    padding-inline: 9px;
    font-size: 8px;
  }

  .copy-button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
    transform: none;
  }

  .copy-button.copied {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .copy-button svg {
    width: 12px;
    height: 12px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .supporter-record {
    display: grid;
    border-bottom: 1px solid var(--line-soft);
  }

  .supporter-record > header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 15px;
    padding: 15px 18px;
    border-bottom: 1px solid var(--line-soft);
  }

  .supporter-record header > div {
    display: grid;
    gap: 4px;
  }

  .supporter-record dl {
    display: grid;
    grid-template-columns:
      repeat(4, minmax(0, 1fr));
    margin: 0;
  }

  .supporter-record dl > div {
    display: grid;
    min-width: 0;
    gap: 4px;
    padding: 13px 16px;
    border-right: 1px solid var(--line-soft);
  }

  .supporter-record dl > div:last-child {
    border-right: 0;
  }

  .supporter-record dt {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    text-transform: uppercase;
  }

  .supporter-record dd {
    overflow-wrap: anywhere;
    margin: 0;
    color: var(--text-soft);
    font-size: 9px;
  }

  .supporter-loading {
    padding: 16px 18px;
    color: var(--text-muted);
    font-size: 9px;
  }

  .sign-out-section {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    padding: 18px;
  }

  .sign-out-section > div {
    display: flex;
    align-items: center;
    gap: 11px;
  }

  .sign-out-section__icon {
    display: grid;
    width: 38px;
    height: 38px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--danger-line);
    border-radius: 9px;
    background: var(--danger-faint);
    color: var(--danger);
  }

  .sign-out-section__icon svg {
    width: 18px;
    height: 18px;
  }

  .sign-out-section > div > div {
    display: grid;
    gap: 4px;
  }

  .sign-out-section strong {
    color: var(--text-soft);
    font-size: 10.5px;
  }

  .sign-out-section p {
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.5;
  }

  .sign-out-button {
    flex: 0 0 auto;
  }

  /* =====================================================
     MODAL
     ===================================================== */

  .modal-layer {
    position: fixed;
    inset: 0;
    z-index: 100;
    display: grid;
    place-items: center;
    padding: 20px;
  }

  .modal-backdrop {
    position: absolute;
    inset: 0;
    width: 100%;
    min-height: 100%;
    border: 0;
    border-radius: 0;
    background: rgba(0, 0, 0, 0.72);
    padding: 0;
    cursor: default;
    backdrop-filter: blur(3px);
  }

  .modal-backdrop:hover:not(:disabled) {
    background: rgba(0, 0, 0, 0.72);
    transform: none;
  }

  .confirm-modal {
    position: relative;
    z-index: 1;
    display: grid;
    width: min(100%, 440px);
    max-width: none;
    justify-items: start;
    gap: 14px;
    margin: 0;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    color: var(--text);
    padding: 20px;
    box-shadow: var(--shadow-deep);
  }

  .confirm-modal:focus {
    outline: none;
  }

  .confirm-modal::backdrop {
    background: transparent;
  }

  .confirm-modal__icon {
    display: grid;
    width: 39px;
    height: 39px;
    place-items: center;
    border: 1px solid var(--danger-line);
    border-radius: 9px;
    background: var(--danger-faint);
    color: var(--danger);
  }

  .confirm-modal__icon svg {
    width: 18px;
    height: 18px;
  }

  .confirm-modal__content {
    display: grid;
    gap: 7px;
  }

  .confirm-modal__content h2 {
    color: var(--text);
    font-size: 17px;
    font-weight: 620;
  }

  .confirm-modal__content p {
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.65;
  }

  .confirm-modal__actions {
    display: flex;
    width: 100%;
    justify-content: flex-end;
    gap: 8px;
    padding-top: 15px;
    border-top: 1px solid var(--line-soft);
  }

  /* =====================================================
     RESPONSIVE
     ===================================================== */

  @media (prefers-reduced-motion: reduce) {
    .account-page,
    .settings-panel {
      animation: none;
    }

    .completion__track > span {
      transition: none;
    }
  }

  @media (max-width: 980px) {
    .account-shell {
      grid-template-columns:
        235px minmax(0, 1fr);
    }

    .public-profile-layout {
      grid-template-columns: 1fr;
    }

    .public-profile-settings {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .profile-preview__body {
      min-height: 280px;
    }

    .password-guidance {
      grid-template-columns: 1fr;
    }
  }

  @media (max-width: 820px) {
    .account-shell {
      grid-template-columns: 1fr;
    }

    .account-navigation {
      position: static;
    }

    .identity-card {
      grid-template-columns:
        minmax(0, 1fr)
        minmax(180px, 0.45fr);
      align-items: center;
    }

    .identity-supporter {
      grid-column: 1;
    }

    .completion {
      grid-column: 2;
      grid-row: 1 / span 2;
    }

    .settings-navigation {
      grid-template-columns:
        repeat(4, minmax(0, 1fr));
      border-top: 1px solid var(--line-soft);
    }

    .settings-navigation button {
      grid-template-columns: 1fr;
      justify-items: center;
      min-height: 72px;
      text-align: center;
    }

    .settings-navigation button > span:nth-child(2) {
      justify-items: center;
    }

    .settings-navigation button small,
    .settings-navigation .visibility-state,
    .settings-navigation .session-dot,
    .settings-navigation .unsaved-dot {
      display: none;
    }

    .account-identifier {
      grid-template-columns: 1fr;
    }
  }

  @media (max-width: 700px) {
    .page-header {
      align-items: stretch;
      flex-direction: column;
    }

    .header-account {
      width: 100%;
    }

    .field-grid,
    .session-overview {
      grid-template-columns: 1fr;
    }

    .session-overview article {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .session-overview article:last-child {
      border-bottom: 0;
    }

    .supporter-record dl {
      grid-template-columns:
        repeat(2, minmax(0, 1fr));
    }

    .supporter-record dl > div:nth-child(2) {
      border-right: 0;
    }

    .supporter-record dl > div:nth-child(-n + 2) {
      border-bottom: 1px solid var(--line-soft);
    }

    .form-actions,
    .sign-out-section {
      align-items: stretch;
      flex-direction: column;
    }

    .form-actions button,
    .sign-out-button {
      width: 100%;
    }

    .public-actions > div {
      align-items: flex-start;
      flex-direction: column;
    }
  }

  @media (max-width: 560px) {
    .identity-card {
      grid-template-columns: 1fr;
    }

    .completion {
      grid-column: 1;
      grid-row: auto;
    }

    .settings-navigation {
      grid-template-columns:
        repeat(2, minmax(0, 1fr));
    }

    .avatar-editor {
      grid-template-columns: 1fr;
      justify-items: start;
    }

    .visibility-control {
      align-items: flex-start;
      flex-direction: column;
    }

    .switch-control {
      width: 100%;
    }

    .settings-panel__header {
      align-items: flex-start;
      flex-direction: column;
    }

    .supporter-record dl {
      grid-template-columns: 1fr;
    }

    .supporter-record dl > div,
    .supporter-record dl > div:nth-child(2) {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .supporter-record dl > div:last-child {
      border-bottom: 0;
    }

    .sign-out-section > div {
      align-items: flex-start;
    }
  }

  @media (max-width: 460px) {
    .settings-form,
    .public-profile-settings {
      padding: 16px;
    }

    .form-actions {
      margin:
        3px -16px -16px;
      padding: 14px 16px;
    }

    .avatar-actions {
      display: grid;
      width: 100%;
    }

    .avatar-actions button {
      width: 100%;
    }

    .user-id-field {
      grid-template-columns: 1fr;
    }

    .copy-button {
      width: 100%;
    }

    .confirm-modal__actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .confirm-modal__actions button {
      width: 100%;
    }
  }
</style>
