import { invoke } from '@tauri-apps/api/core';
import { Instance } from '../types';

/**
 * API service for communicating with the Tauri backend
 * This provides a bridge between the React frontend and the existing C++ MultiMC backend
 */

export class MinecraftAPI {
  /**
   * Get all Minecraft instances
   */
  static async getInstances(): Promise<Instance[]> {
    try {
      return await invoke('get_instances');
    } catch (error) {
      console.error('Failed to get instances:', error);
      return [];
    }
  }

  /**
   * Launch a Minecraft instance
   */
  static async launchInstance(instanceId: string): Promise<boolean> {
    try {
      return await invoke('launch_instance', { instanceId });
    } catch (error) {
      console.error('Failed to launch instance:', error);
      return false;
    }
  }

  /**
   * Create a new instance
   */
  static async createInstance(name: string, version: string): Promise<string | null> {
    try {
      return await invoke('create_instance', { name, version });
    } catch (error) {
      console.error('Failed to create instance:', error);
      return null;
    }
  }

  /**
   * Delete an instance
   */
  static async deleteInstance(instanceId: string): Promise<boolean> {
    try {
      return await invoke('delete_instance', { instanceId });
    } catch (error) {
      console.error('Failed to delete instance:', error);
      return false;
    }
  }

  /**
   * Get instance details
   */
  static async getInstanceDetails(instanceId: string): Promise<Instance | null> {
    try {
      return await invoke('get_instance_details', { instanceId });
    } catch (error) {
      console.error('Failed to get instance details:', error);
      return null;
    }
  }

  /**
   * Update instance settings
   */
  static async updateInstance(instanceId: string, settings: Partial<Instance>): Promise<boolean> {
    try {
      return await invoke('update_instance', { instanceId, settings });
    } catch (error) {
      console.error('Failed to update instance:', error);
      return false;
    }
  }

  /**
   * Get available Minecraft versions
   */
  static async getAvailableVersions(): Promise<string[]> {
    try {
      return await invoke('get_available_versions');
    } catch (error) {
      console.error('Failed to get available versions:', error);
      return [];
    }
  }

  /**
   * Install mods to an instance
   */
  static async installMod(instanceId: string, modPath: string): Promise<boolean> {
    try {
      return await invoke('install_mod', { instanceId, modPath });
    } catch (error) {
      console.error('Failed to install mod:', error);
      return false;
    }
  }

  /**
   * Get Java installations
   */
  static async getJavaVersions(): Promise<Array<{ path: string; version: string }>> {
    try {
      return await invoke('get_java_versions');
    } catch (error) {
      console.error('Failed to get Java versions:', error);
      return [];
    }
  }

  /**
   * Check for launcher updates
   */
  static async checkForUpdates(): Promise<{ available: boolean; version?: string }> {
    try {
      return await invoke('check_for_updates');
    } catch (error) {
      console.error('Failed to check for updates:', error);
      return { available: false };
    }
  }
}

/**
 * Settings API for managing launcher settings
 */
export class SettingsAPI {
  static async getSettings(): Promise<Record<string, any>> {
    try {
      return await invoke('get_settings');
    } catch (error) {
      console.error('Failed to get settings:', error);
      return {};
    }
  }

  static async updateSettings(settings: Record<string, any>): Promise<boolean> {
    try {
      return await invoke('update_settings', { settings });
    } catch (error) {
      console.error('Failed to update settings:', error);
      return false;
    }
  }
}

/**
 * Account API for managing Minecraft accounts
 */
export class AccountAPI {
  static async getAccounts(): Promise<Array<{ id: string; username: string; type: string }>> {
    try {
      return await invoke('get_accounts');
    } catch (error) {
      console.error('Failed to get accounts:', error);
      return [];
    }
  }

  static async addAccount(username: string, password: string): Promise<boolean> {
    try {
      return await invoke('add_account', { username, password });
    } catch (error) {
      console.error('Failed to add account:', error);
      return false;
    }
  }

  static async removeAccount(accountId: string): Promise<boolean> {
    try {
      return await invoke('remove_account', { accountId });
    } catch (error) {
      console.error('Failed to remove account:', error);
      return false;
    }
  }
}
