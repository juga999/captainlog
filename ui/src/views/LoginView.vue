<script setup lang="ts">

import { ref } from 'vue';

import LoginParams from '../models/LoginParams';

import Service from '../services/Service'

const response = ref<string>('');

const loginParams = LoginParams.newReactive('', '');

const service = new Service();

async function onSubmit() {
  if (loginParams.isValid()) {
    console.log('Logging in with', loginParams.email, loginParams.password);
    // Handle actual login logic here (e.g., API call)
    // https://api.wid-timer.org/signin
    // email, password
    let r = await service.post<LoginParams>('/signin', loginParams);
    response.value = r;
  } else {
    console.error('Email and password are required');
  }
}

</script>

<template>
  <div class="login-container">
    <h2>Login</h2>
    {{ loginParams }}
    <form @submit.prevent="onSubmit">
      <div class="input-group">
        <label for="email">Email:</label>
        <input type="email" v-model="loginParams.email" id="email" required />
      </div>
      <div class="input-group">
        <label for="password">Password:</label>
        <input type="password" v-model="loginParams.password" id="password" required />
      </div>
      <button type="submit">Login</button>
    </form>
    {{response}}
  </div>
</template>

<style scoped>
  .login-container {
    max-width: 400px;
    margin: 0 auto;
    padding: 1rem;
    border: 1px solid #ccc;
    border-radius: 8px;
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
  }
  .input-group {
    margin-bottom: 1rem;
  }
  input {
    width: 100%;
    padding: 0.5rem;
    margin-top: 0.25rem;
    border: 1px solid #ccc;
    border-radius: 4px;
  }
  button {
    width: 100%;
    padding: 0.75rem;
    background-color: #42b983;
    border: none;
    color: white;
    font-size: 1rem;
    cursor: pointer;
    border-radius: 4px;
  }
  button:hover {
    background-color: #38a169;
  }
  </style>
  